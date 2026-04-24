#!/usr/bin/env python3
"""
ORION Test Reporter
Reads the JSON log produced by the C++ test engine and produces
a human-readable report with pass/fail summary and anomaly timeline.
"""

import json
import sys
import os
from datetime import datetime

# Why argparse? Because hardcoding the path makes this script
# useless anywhere except your machine. Configurable = reusable.
import argparse


def load_results(filepath: str) -> dict:
    """Load and validate the JSON results file."""
    if not os.path.exists(filepath):
        print(f"[ERROR] Results file not found: {filepath}")
        sys.exit(1)

    with open(filepath, 'r') as f:
        try:
            return json.load(f)
        except json.JSONDecodeError as e:
            # Why catch this specifically?
            # A corrupted JSON file would give a cryptic Python error.
            # This gives a clear message pointing to the real problem.
            print(f"[ERROR] Failed to parse JSON: {e}")
            sys.exit(1)


def print_header():
    """Print report header with timestamp."""
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print("=" * 60)
    print("  ORION TEST REPORT")
    print(f"  Generated: {now}")
    print("=" * 60)


def print_summary(summary: dict):
    """Print the test summary section."""
    print("\n[ SUMMARY ]")
    print(f"  Total run    : {summary['total_run']}")
    print(f"  Passed       : {summary['passed']}")
    print(f"  Failed       : {summary['failed']}")
    print(f"  Errors       : {summary['errors']}")
    print(f"  Anomalies    : {summary['anomaly_count']}")

    # Pass rate — why calculate this?
    # A raw count of failures means nothing without context.
    # 6 failures out of 6 is catastrophic. 6 out of 600 is noise.
    total = summary['total_run']
    if total > 0:
        pass_rate = (summary['passed'] / total) * 100
        print(f"  Pass rate    : {pass_rate:.1f}%")


def print_anomaly_timeline(summary: dict):
    """Print the anomaly event timeline."""
    events = summary.get('anomaly_events', [])

    print("\n[ ANOMALY TIMELINE ]")
    if not events:
        print("  No anomalies detected. ✓")
        return

    for event in events:
        arrow = f"{event['from']} → {event['to']}"
        print(f"  t={event['timestamp_ms']:>6}ms  {arrow:<30}  {event['reason']}")


def print_failures(frames: list):
    """Print only the frames that had failures or errors."""
    failures = [
        f for f in frames
        if any(r['status'] in ('FAIL', 'ERROR') for r in f['results'])
    ]

    print("\n[ FAILED FRAMES ]")
    if not failures:
        print("  No failures detected. ✓")
        return

    for frame in failures:
        print(f"\n  t={frame['timestamp_ms']}ms  "
              f"value={frame['value']:.4f}  "
              f"hw_valid={frame['hardware_valid']}  "
              f"state={frame['detector_state']}")
        for result in frame['results']:
            if result['status'] in ('FAIL', 'ERROR'):
                print(f"    [{result['status']}] {result['name']}: {result['message']}")


def determine_exit_code(summary: dict) -> int:
    """
    Return 0 if all tests passed, 1 if any failed.
    Why a function? Because the exit logic might get more complex —
    maybe warnings return 2, critical failures return 3.
    Isolating it means one place to change.
    """
    if summary['failed'] > 0 or summary['errors'] > 0:
        return 1
    return 0


def main():
    parser = argparse.ArgumentParser(description='ORION Test Reporter')
    parser.add_argument(
        '--input',
        default='logs/run.json',
        help='Path to the JSON results file (default: logs/run.json)'
    )
    args = parser.parse_args()

    # Load data
    data     = load_results(args.input)
    frames   = data.get('frames', [])
    summary  = data.get('summary', {})

    # Print report sections
    print_header()
    print_summary(summary)
    print_anomaly_timeline(summary)
    print_failures(frames)

    # Final verdict
    exit_code = determine_exit_code(summary)
    print("\n" + "=" * 60)
    if exit_code == 0:
        print("  RESULT: ALL TESTS PASSED ✓")
    else:
        print("  RESULT: TESTS FAILED ✗")
    print("=" * 60 + "\n")

    sys.exit(exit_code)


if __name__ == '__main__':
    # Why this guard?
    # This block only runs when the script is executed directly.
    # If another Python script imports this file, main() won't
    # run automatically. That makes the reporter importable as a
    # module for future use — testable, reusable.
    main()