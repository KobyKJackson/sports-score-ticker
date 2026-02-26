"""Shared utility functions for the sports score ticker."""

from datetime import datetime

import pytz
from tzlocal import get_localzone


def convert_to_local_time(utc_time: datetime) -> datetime:
    """Convert a naive UTC datetime to the system's local timezone."""
    local_timezone = get_localzone()
    utc_time = utc_time.replace(tzinfo=pytz.utc)
    return utc_time.astimezone(local_timezone)
