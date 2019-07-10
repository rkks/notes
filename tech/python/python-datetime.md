# Intro
Most of us have faced a point when trying to make things work with the Python
`datetime` module where we resort to guess-and-check until the errors go away.
`datetime` is one of those APIs
that seems easy to use but requires the developer to have a deep understanding
of what things actually mean, as otherwise it is really easy to introduce unexpected bugs
given the actual complexity of date and time related issues.

# Time Standards
The first concept we need to grasp when working with time is
a standard that defines how we can measure units of time. The same way we have standards to measure weight or length that define kilograms or meters, we need a way to accurately define what
a second means. We can then use other time references like days, weeks or years using
a calendar standard as multiples of the second.

## UT1
One of the "simplest" ways to measure a second is just as a fraction of the day
given that we can reliably guarantee that the sun will rise and set everyday (in most places).
This gave birth to Universal Time (UT1), the successor of GMT. Nowadays we use stars and quasars
to measure how long it takes for the earth to perform a full rotation.
Even if this seems precise enough it still brings some issues along as due to
the gravitation of the moon, tides and earthquakes the days change length along
the year.
Even if this is not an issue for most applications, it becomes a non trivial problem when we
require really precise measurements - GPS triangulation is a good example of a very time sensitive process, where a second offset result in a completly different location on the globe.

## TAI
As a result the International Atomic Time (TAI) was designed to be as precise as possible. Using [atomic clocks](https://en.wikipedia.org/wiki/Atomic_clock) in multiple laboratories across the earth we get the most accurate and constant measure of the second which allows us to compute time intervals with the highest precision.

But this precision is both a blessing and a curse as TAI is so precise that it deviates from UT1 or what we call "Civil Time". This means that we will eventually have our clock noon deviate substantially from the solar noon.

## UTC
That gave birth to Coordinated Universal Time (UTC), which brought together the best of the two worlds. It uses the measurement of a second as defined by TAI which allows for accurate measures of time and introduces leap seconds, ensuring that it never deviates from UT1 by more than 0.9 seconds.

## How all this plays together in your computer
With all this information the reader should be able now to understand how the OS is serving the time to them at a specific moment.

Note that the computer has no atomic clock inside but uses an internal clock synchronized with the rest of the world via [NTP](http://www.ntp.org/).

The most common way in Unix-like systems is to use the [POSIX time](https://en.wikipedia.org/wiki/Unix_time). Which is defined as the number of seconds since the Unix epoch (1970) without taking leap seconds into account. As this time does not expose the leap second nor does Python, some companies have defined their own way of handling the time by smearing the leap second across the time around it through their NTP server. See [Google time as an example](https://developers.google.com/time/smear).

# Timezones
![Credit: WikiMedia](https://upload.wikimedia.org/wikipedia/commons/e/e8/Standard_World_Time_Zones.png)

We have seen what UTC is and how it allows us to define dates and times but countries like to have their wall time noon to match with the solar time for noon so the sun is on the top of the sky at 12pm.
That is why UTC defines offsets so we can have 12am with an offset of +4 hours from UTC which effectively means that the actual time without offset is 8am.
Governments define the standard offset from UTC that a geographical position follows, which effectively creates a timezone.
The most comon database for timezones is known as the [Olson Database](https://en.wikipedia.org/wiki/Tz_database). This can be retrieved in Python using [`dateutil.tz`](https://dateutil.readthedocs.io/en/stable/tz.html).

```python
>>> from dateutil.tz import gettz
>>> gettz("Europe/Madrid")
```

The result of gettz gives us an object that we can use to create timezone aware dates in Python.

```python
>>> import datetime as dt
>>> dt.datetime.now().isoformat()
'2017-04-15T14:16:56.551778'  # This is a naive datetime
>>> dt.datetime.now(gettz("Europe/Madrid")).isoformat()
'2017-04-15T14:17:01.256587+02:00'  # This is a tz aware datetime, always prefer these
```

We can see how the second time we get the current time via the [now](https://docs.python.org/3.6/library/datetime.html#datetime.datetime.now) function of datetime we pass a [tzinfo](https://docs.python.org/3.6/library/datetime.html#datetime.tzinfo) object and the offset is sticked into the [ISO string representation](https://en.wikipedia.org/wiki/ISO_8601) of that datetime.

Should we want to use just plain UTC in Python 3 we don't need any external libraries:

```python
>>> dt.datetime.now(dt.timezone.utc).isoformat()
'2017-04-15T12:22:06.637355+00:00'
```

# DST
Once we grasp all this knowledge we might feel prepared to work with timezones but we need to be aware of one more thing that happens only in some timezones: Daylight Saving Time (DST).
The countries that follow the DST will move their clocks one hour forward in spring and one hour backwards in autumn back to the standard time of the timezone.

This effectively implies that a single timezone can have multiple offsets as we can see in the following example:

```python
>>> dt.datetime(2017, 7, 1, tzinfo=dt.timezone.utc).astimezone(gettz("Europe/Madrid""))
'2017-07-01T02:00:00+02:00’
>>> dt.datetime(2017, 1, 1, tzinfo=dt.timezone.utc).astimezone(gettz("Europe/Madrid"))
'2017-01-01T01:00:00+01:00'
```

This gives us days that are made of 23 or 25 hours resulting in really interesting time arithmetics.
Depending on the time and the timezone, adding a day does not necessarily mean to add 24 hours.

```python
>>> today = dt.datetime(2017, 10, 29, tzinfo=gettz("Europe/Madrid"))
>>> tomorrow = today + dt.timedelta(days=1)
>>> tomorrow.astimezone(dt.timezone.utc) - today.astimezone(dt.timezone.utc)
datetime.timedelta(1, 3600)  # We've added 25 hours
```

The best strategy here -when working with timestamps- is to use non DST-aware timezones (ideally UTC+00:00).

# Serializing your datetime objects

The day will come that you need to send your `datetime` objects in [JSON](http://www.json.org/) and you will get the following:

```python
>>> now = dt.datetime.now(dt.timezone.utc)
>>> json.dumps(now)
TypeError: Object of type 'datetime' is not JSON serializable
```

There are three main ways to serialize `datetime` in JSON

## String

`datetime` has two main functions to convert to and from a string given an specific format: [`strftime` and `strptime`](https://docs.python.org/2/library/datetime.html#strftime-strptime-behavior). But the best way is to use the standard [ISO_8601](https://en.wikipedia.org/wiki/ISO_8601) for serializing time related objects as string, this is exposed by just calling `isoformat` on the `datetime` object.

```python
>>> now = dt.datetime.now(gettz("Europe/London"))
>>> now.isoformat()
'2017-04-19T22:47:36.585205+01:00'
```

To get a `datetime` object from a string that was formatted using isofomat with an UTC timezone we can just rely on strptime:

```python
>>> dt.datetime.strptime(now_str, "%Y-%m-%dT%H:%M:%S.%f+00:00").replace(tzinfo=dt.timezone.utc)
datetime.datetime(2017, 4, 19, 21, 49, 5, 542320, tzinfo=datetime.timezone.utc)
```

In this example we are just hardcoding the offset to be UTC and then setting it once the `datetime` object is created. A better way to fully parse the string including timezones is using the external library [`dateutil`](https://dateutil.readthedocs.io/en/stable/)

```python
>>> from dateutil.parser import parse
>>> parse('2017-04-19T21:49:05.542320+00:00')
datetime.datetime(2017, 4, 19, 21, 49, 5, 542320, tzinfo=tzutc())
>>> parse('2017-04-19T21:49:05.542320+01:00')
datetime.datetime(2017, 4, 19, 21, 49, 5, 542320, tzinfo=tzoffset(None, 3600))
```

Note once we serialize and deserialize we lose the timezone information and keep only the offset.

## Integer

We are able to store a datetime as an integer by just using the number of seconds that passed since a specific epoch (reference date). The most known epoch in computer system is known as the [UNIX Epoch](https://en.wikipedia.org/wiki/Unix_time), which references the first second since 1970. This means that "5" representes the 5th second on the first of January of 1970.

The python standard library provides us with tools to get the current time as UNIX time and to transform between datetime objects and their int representations as UNIX time.


Getting the current time as an integer:

```python
>>> import datetime as dt
>>> from dateutil.tz import gettz
>>> import time
>>> unix_time = time.time()
```

UNIX time to datetime:

```python
>>> unix_time
1492636231.597816
>>> datetime = dt.datetime.fromtimestamp(unix_time, gettz("Europe/London"))
>>> datetime.isoformat()
'2017-04-19T22:10:31.597816+01:00'
```

Getting the UNIX time given a datetime:
```python
>>> time.mktime(datetime.timetuple())
1492636231.0
>>> # or using the calendar library
>>> calendar.timegm(datetime.timetuple())
```

## Objects

The last option is just to serialize the object itself as an object that will give special meaning at decoding time.

```python
import datetime as dt
from dateutil.tz import gettz, tzoffset

def json_to_dt(obj):
    if obj.pop('__type__', None) != "datetime":
        return obj
    zone, offset = obj.pop("tz")
    obj["tzinfo"] = tzoffset(zone, offset)
    return dt.datetime(**obj)

def dt_to_json(obj):
    if isinstance(obj, dt.datetime):
        return {
            "__type__": "datetime",
            "year": obj.year,
            "month" : obj.month,
            "day" : obj.day,
            "hour" : obj.hour,
            "minute" : obj.minute,
            "second" : obj.second,
            "microsecond" : obj.microsecond,
            "tz": (obj.tzinfo.tzname(obj), obj.utcoffset().total_seconds())
        }
    else:
        raise TypeError("Cant serialize {}".format(obj))
```

Now we can encode JSON:
```python
>>> import json
>>> now = dt.datetime.now(dt.timezone.utc)
>>> json.dumps(now, default=dt_to_json)  # From datetime
'{"__type__": "datetime", "year": 2017, "month": 4, "day": 19, "hour": 22, "minute": 32, "second": 44, "microsecond": 778735, "tz": "UTC"}'
>>> # Also works with timezones
>>> now = dt.datetime.now(gettz("Europe/London"))
>>> json.dumps(now, default=dt_to_json)
'{"__type__": "datetime", "year": 2017, "month": 4, "day": 19, "hour": 23, "minute": 33, "second": 46, "microsecond": 681533, "tz": "BST"}'
```

And decode it as well:

```python
>>> input_json='{"__type__": "datetime", "year": 2017, "month": 4, "day": 19, "hour": 23, "minute": 33, "second": 46, "microsecond": 681533, "tz": "BST"}'
>>> json.loads(input_json, object_hook=json_to_dt)
datetime.datetime(2017, 4, 19, 23, 33, 46, 681533, tzinfo=tzlocal())
>>> input_json='{"__type__": "datetime", "year": 2017, "month": 4, "day": 19, "hour": 23, "minute": 33, "second": 46, "microsecond": 681533, "tz": "EST"}'
>>> json.loads(input_json, object_hook=json_to_dt)
datetime.datetime(2017, 4, 19, 23, 33, 46, 681533, tzinfo=tzfile('/usr/share/zoneinfo/EST'))
>>> json.loads(input_json, object_hook=json_to_dt).isoformat()
'2017-04-19T23:33:46.681533-05:00'
```



# Wall times

After this the reader might be tempted to just convert all datetime objects to UTC and work only with UTC datetimes and fixed offsets. Even if this is by far the best approach for timestamps, it quickly breaks for future wall times.

We can distinguish two main types of _time_ points. Wall times and timestamps. Timestamps are universal points in time not related to anywhere in particular. Examples include the time a star is born or when a line is logged to a file. But things change when we speak about the time "we read on the wall clock". When we say "see you tomorrow at two" we are not referring to UTC offsets but to tomorrow at 2pm on my local timezone not matter what the offset at this point is. We cannot just map those wall times to timestaps -we can for past ones- as for future occurrences countries might change their offset, which believe it or not happens more frequent than it seems.

For those situations we need to save the datetime with the timezone it refers to and not the offset.

# Differences when working with pytz

Since Python 3.6 the recommended library to get the Olson database is [`dateutil`](https://dateutil.readthedocs.io/en/stable/), but it used to be [`pytz`](http://pytz.sourceforge.net/).

They might seem similar but their approach to handling timezones is quite different in some situations. Getting the current time is quite simple as well.

```python
>>> import pytz
>>> dt.datetime.now(pytz.timezone("Europe/London"))
datetime.datetime(2017, 4, 20, 0, 13, 26, 469264, tzinfo=<DstTzInfo 'Europe/London' BST+1:00:00 DST>)
```

But a common pitfall with pytz it to pass a pytz timezone as a tzinfo attribute of a datetime.

```python
>>> dt.datetime(2017, 5, 1, tzinfo=pytz.timezone("Europe/Helsinki"))
datetime.datetime(2017, 5, 1, 0, 0, tzinfo=<DstTzInfo 'Europe/Helsinki' LMT+1:40:00 STD>)
>>> pytz.timezone("Europe/Helsinki").localize(dt.datetime(2017, 5, 1, is_dst=None))
datetime.datetime(2017, 5, 15, 1, 0, tzinfo=<DstTzInfo 'Europe/Helsinki' EEST+3:00:00 DST>)
```

We should always call localize on the `datetime` objects we build as `pytz` will assign the first offset it finds for the timezone otherwise.

Another major difference can be found when performing time arithmetics. Whilst we saw that in `dateutil` the additions worked as if we were adding wall time in the specified timezone, when the datetime object has a `pytz` tzinfo absolute hours are added and the caller needs to call `normalize` after the operation. Example:

```python
>>> today = dt.datetime(2017, 10, 29)
>>> tz = pytz.timezone("Europe/Madrid")
>>> today = tz.localize(dt.datetime(2017, 10, 29), is_dst=None)
>>> tomorrow = today + dt.timedelta(days=1)
>>> tomorrow
datetime.datetime(2017, 10, 30, 0, 0, tzinfo=<DstTzInfo 'Europe/Madrid' CEST+2:00:00 DST>)
>>> tz.normalize(tomorrow)
datetime.datetime(2017, 10, 29, 23, 0, tzinfo=<DstTzInfo 'Europe/Madrid' CET+1:00:00 STD>)
```

Note that with the `pytz` tzinfo it has added 24 absolute hours (23 hours on the wall time).

The following table resumes the way to get either wall/timestamps arithmetics with both `pytz` and `dateutil`.

|           | pytz           | dateutil  |
| ------------- |:-------------:| -----:|
| wall time | `obj.tzinfo.localize(obj.replace(tzinfo=None) + timedelta, is_dst=is_dst)` | `obj + timedelta` |
| absolute time | `obj.tzinfo.normalize(obj + timedelta)` | `(obj.astimezone(pytz.utc) + timedelta).astimezone(obj.tzinfo)` |

Note that adding wall times can lead to unexpected results when DST changes occur.

Last but not least dateutil plays nicely with the fold attribute added in [PEP0495](https://www.python.org/dev/peps/pep-0495/) and provides backward compatibility if using earlier versions of Python.

# Quick tips
After all this, how should we avoid the common issues when working with time?

- Always use timezones, don't rely on implicit local timezone.
- Use dateutil/pytz to handle timezones.
- Always use UTC if working with timestamps.
- Remember that a day is not always made of 24h for some timezones.
- Keep up to date your timezone database.
- Always test your code against situations like DST changes.

# Libraries worth mentioning

- [dateutil](https://dateutil.readthedocs.io/): Multiple utilities to work with time
- [freezegun](https://github.com/spulec/freezegun): Easier testing of time related applications
- [arrow](http://arrow.readthedocs.io/en/latest/)/[pendulum](https://pendulum.eustace.io/): Drop in replacement of the standard datetime module
- [astropy](http://docs.astropy.org/en/stable/time/): Useful for astronomical times and working with leap seconds
