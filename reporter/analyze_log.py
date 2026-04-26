#!/usr/bin/env python3

"""
ORION Log Analyzer
Reads a raw test log and extracts signal from noise.
Usage: python3 analyze_log.py --input reporter/night_run.log
"""

import argparse
import re
from dataclasses import dataclass
from typing import Optional
from collections import defaultdict

# Why dataclass? 
#  Instead of working with raw strings everywhere, we parse once into structured objects.
#  The rest of the code works with clean typed data -- no fragile string splits.

@dataclass
class LogEntry:
    timestamp:  str
    level:      str             # INFO, ERROR, WARN
    kind:       str             # Test or system
    name:       str             # test name or system event
    result:     Optional[str]   # PASS, FAIL, None for system
    duration_ms:Optional[str]   # None for System entries
    reason:     Optional[str]   # sensor_timeout, etc
    raw:        str             # original line - always keep raw data

def parse_line(line: str) -> Optional[LogEntry]:
    """
    Parse one log line into a LogEntry.
    Return None if the line doesn't match our format.
    """
    line = line.strip()
    if not line:
        return None
    

    """
    Why regex?
    Because log formats are messy. A simple split() breaks on inconsistent spacing.
    Regex lets us name exactly what we are looking for.
    """
    pattern = re.compile(
        r'(?P<timestamp>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+)'
        r'\s+\[(?P<level>\w+)\]'
        r'\s+(?P<kind>TEST|SYSTEM)'
        r'\s+(?P<name>\S+)'
        r'\s+(?P<rest>.*)'
    )

    match = pattern.match(line)
    if not match:
        return None
    
    timestamp   = match.group('timestamp')
    level       = match.group('level')
    kind        = match.group('kind')
    name        = match.group('name')
    rest        = match.group('rest').strip()

    result      = None
    duration_ms = None
    reason      = None

    if kind == 'TEST':
        # Parse: PASS/FAIL Nms reason=X
        parts = rest.split()
        if parts:
            result = parts[0] # PASS or FAIL
            if len(parts) > 1 :
                dur_str = parts[1].replace('ms', '')
                try:
                    duration_ms = int(dur_str)
                except ValueError:
                    pass
    
        # Look for reason:
        reason_match = re.search(r'reason=(\S+)', rest)
        if reason_match:
            reason = reason_match.group(1)

    return LogEntry(
        timestamp=timestamp,
        level=level,
        kind=kind,
        name=name,
        result=result,
        duration_ms=duration_ms,
        reason=reason,
        raw=line
    )

def load_log(filepath: str) -> list[LogEntry]:
    """ Load and parse every line in the log file. """
    entries = []
    with open(filepath, 'r') as f:
        for line in f:
            entry = parse_line(line)
            if entry:
                entries.append(entry)
    return entries



def print_summary (entries: list[LogEntry]):
    """ Stage 2: Overall PASS/FAIL summary per test name."""
    print("\n[ SUMMARY BY TEST ]")
    print(f" {'TEST NAME':<25} {'RUNS':>5} {'PASS':>5} {'FAIL':>7} {'AVG MS':>8}")
    print(" " + "-"*60)

    # Group results by test name
    # defaultdict means we never have to check if a key exists first
    stats = defaultdict(lambda: {'pass':0, 'fail':0, 'durations':[]})

    for e in entries:
        if e.kind != 'TEST':
            continue
        if e.result == 'PASS':
            stats[e.name]['pass']+=1
        elif e.result == 'FAIL':
            stats[e.name]['fail']+=1
        
        if e.duration_ms is not None:
            stats[e.name]['durations'].append(e.duration_ms)

    
    for name, s in sorted(stats.items()):
        runs        = s['pass'] + s['fail']
        fail_pct    = (s['fail']/runs * 100) if runs > 0 else 0
        avg_ms      = (sum(s['durations']) / len(s['durations'])) if s['durations'] else 0
        # Flag high failure rates:
        flag        = " ⚠️" if fail_pct > 30 else " "

        print(f" {name:<25} {runs:>5} {s['pass']:>5} {s['fail']:>5} " f"{fail_pct:>6.1f}% {avg_ms:>7.1f}ms {flag}")


def print_flakiness(entries: list[LogEntry]):
    """
        Stage 3: Detect Flaky States:
        A flaky test is one that both passes AND fails. --
        same test, different outcomes, no code change.
    """

    print("\n [ FLAKINESS REPORT ]")
    results_by_test = defaultdict(set)
    for e in entries:
        if e.kind == 'TEST' and e.result:
            results_by_test[e.name].add(e.result)

    
    flaky = {name: results for name, results in results_by_test.items()
             if 'PASS' in results and 'FAIL' in results}
    
    if not flaky:
        print("   ✓  No flaky tests detected. ✓   ")
        return
    
    for name in sorted(flaky.keys()) :
        print(f"    ⚠️ {name}   - both PASS and FAILED detected.")


def print_spike_correlation(entries: list[LogEntry]) :
    """
        Stage 4: The most important analysis.
        Do failures clusted in the window after a CPU spike?
        This is what transforms a hunch into an engineering findind.
    """

    print("\n [     CPU SPIKE CORRELATION.      ]")

    # Find CPU spike timestamps
    spikes = [e for e in entries
                if e.kind == 'SYSTEM' and 'cpu_spike' in e.name]
    
    if not spikes :
        print(" No CPU spikes detected in this log.")

    print(f"    Found {len(spikes)} CPU spike(s) in log. \n")

    # For each spike, count failures in the 200ms window after it
    # Why 200ms? That's roughly 2x our frame interval, enough time to see the effect without pulling in unrelated events.
    WINDOW_MS = 200

    for spike in spikes:
        spike_time = spike.timestamp
        print(f"    Spike at: {spike_time}: ")

        # Compare failure rate before vs after spike.
        before_spike = [ e for e in entries
                        if e.kind == 'TEST'
                        and e.timestamp < spike_time]
        before_failures = len([e for e in before_spike if e.result == 'FAIL'])
        before_rate = (before_failures / len(before_spike) * 100) if before_spike else 0

        # Find tests that ran within WINDOW_MS after this spike
        # We compare timestamps as strings, works because our format is lexicographically sortable (ISO 8601)

        after_spike = [
            e for e in entries
            if e.kind == 'TEST'
            and e.timestamp > spike_time
        ] 

        # Take only the first N entries after spike. Simple approximation window.
        window = after_spike[:10]

        failures = [e for e in window if e.result == 'FAIL']
        passes   = [e for e in window if e.result == 'PASS']

        after_failures = len(failures)
        after_rate = (after_failures / len(window)*100) if window else 0


        print(f"    Tests in window after spike: {len(window)}")
        print(f"    Failuers: {len(failures)}")
        print(f"    Passes:   {len(passes)}")
        print(f"    Failure rate BEFORE spike: {before_rate:.1f}%")
        print(f"    Failure rate AFTER spike:  {after_rate:.1f}%")

        # Conclusion:
        if(after_rate > before_rate * 2):
            print(f"    ⚠️ CORRELATION DETECTED:    "
                  f"    majority of post-spike tests failed")
        else :
            print(f"    No strong correlation in this window.")


def main():
    parser = argparse.ArgumentParser(description='ORION Log Analyzer')
    parser.add_argument('--input', required=True, help='Path to log file')
    args = parser.parse_args()

    print('='*60)
    print("     ORION LOG ANALYZER")
    print("="*60)

    entries = load_log(args.input)
    print(f"\n Loaded {len(entries)} log entries from {args.input}")

    print_summary(entries)
    print_flakiness(entries)
    print_spike_correlation(entries)

    print("\n" + "="*60 + "\n")

if __name__ == '__main__':
    main()