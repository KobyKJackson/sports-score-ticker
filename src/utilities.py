import pytz
from tzlocal import get_localzone


def convert_to_local_time(utc_time):
    local_timezone = get_localzone()
    utc_time = utc_time.replace(tzinfo=pytz.utc)
    local_time = utc_time.astimezone(local_timezone)
    return local_time
