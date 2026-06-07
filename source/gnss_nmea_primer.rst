NMEA 0183 Primer
=================

NMEA 0183 is the serial data standard used by virtually all GNSS receivers.
Understanding the sentence structure is necessary to parse the incoming stream
correctly and extract reliable position data.

Sentence Structure
-------------------

Every NMEA sentence follows this format:

.. code-block:: text

   $TALKER SENTENCE , field1 , field2 , ... , fieldN * CHECKSUM \r\n
    ──────  ────────                                   ──────────
    2-char  3-char                                     2-char hex XOR
    talker  sentence                                   of all chars
    id      type                                       between $ and *

**Talker IDs** relevant to multi-constellation receivers:

- ``GP`` — GPS only
- ``GL`` — GLONASS only
- ``GA`` — Galileo only
- ``GB`` / ``GN`` — multi-constellation (most modern receivers use ``GN``)

Key Sentences
--------------

GGA — Global Positioning System Fix Data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The most important sentence — provides position, altitude, fix quality,
and number of satellites:

.. code-block:: text

   $GNGGA,142311.847,4808.2292,N,01134.5674,E,1,09,0.9,524.3,M,47.8,M,,*47

Field breakdown:

.. list-table::
   :header-rows: 1
   :widths: 8 20 72

   * - Field
     - Name
     - Description / Values
   * - 0
     - UTC time
     - ``HHMMSS.sss`` — time of fix
   * - 1
     - Latitude
     - ``DDMM.MMMM`` — degrees + decimal minutes
   * - 2
     - N/S
     - ``N`` or ``S``
   * - 3
     - Longitude
     - ``DDDMM.MMMM``
   * - 4
     - E/W
     - ``E`` or ``W``
   * - 5
     - Fix quality
     - ``0``=no fix, ``1``=GPS fix, ``2``=DGPS, ``4``=RTK fixed, ``5``=RTK float
   * - 6
     - Satellites
     - Number of satellites in use
   * - 7
     - HDOP
     - Horizontal dilution of precision (lower = better)
   * - 8
     - Altitude
     - Altitude above mean sea level, metres
   * - 9
     - Alt unit
     - ``M`` for metres
   * - 10
     - Geoid sep
     - Separation between WGS84 ellipsoid and mean sea level
   * - 14
     - Checksum
     - Two-character hex XOR checksum after ``*``

RMC — Recommended Minimum Specific GNSS Data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Provides position, speed, track, and date — useful for the date field
(GGA only has time-of-day):

.. code-block:: text

   $GNRMC,142311.847,A,4808.2292,N,01134.5674,E,0.00,0.00,070626,,,A*68

.. list-table::
   :header-rows: 1
   :widths: 8 20 72

   * - Field
     - Name
     - Description
   * - 0
     - UTC time
     - ``HHMMSS.sss``
   * - 1
     - Status
     - ``A``=active (valid), ``V``=void (no fix)
   * - 2–5
     - Lat/Lon
     - Same format as GGA
   * - 6
     - Speed
     - Speed over ground, knots
   * - 7
     - Track
     - Track angle, degrees true
   * - 8
     - Date
     - ``DDMMYY``
   * - 11
     - Mode
     - ``A``=autonomous, ``D``=DGPS, ``R``=RTK

GLL — Geographic Position (Lat/Lon)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Simpler position sentence — useful as a cross-check but GGA is preferred.

GSA — GPS DOP and Active Satellites
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Provides PDOP, HDOP, VDOP — useful for fix quality assessment.

Checksum Validation
--------------------

Always validate checksums before using a sentence. The checksum is the XOR
of all bytes between ``$`` and ``*`` (exclusive):

.. code-block:: cpp

   bool validateNmeaChecksum(const QString& sentence)
   {
       // Find $ and *
       int dollarPos = sentence.indexOf('$');
       int starPos   = sentence.lastIndexOf('*');
       if (dollarPos < 0 || starPos < 0 || starPos < dollarPos + 2)
           return false;

       // XOR all bytes between $ and *
       uint8_t checksum = 0;
       for (int i = dollarPos + 1; i < starPos; ++i)
           checksum ^= static_cast<uint8_t>(sentence[i].toLatin1());

       // Parse declared checksum
       bool ok = false;
       uint8_t declared = sentence.mid(starPos + 1, 2).toUInt(&ok, 16);

       return ok && (checksum == declared);
   }

Converting NMEA Lat/Lon to Decimal Degrees
--------------------------------------------

NMEA encodes position as ``DDMM.MMMM`` (degrees + decimal minutes), not
decimal degrees. Convert before storing:

.. code-block:: cpp

   double nmeaToDecimalDegrees(const QString& nmea, const QString& hemisphere)
   {
       if (nmea.isEmpty()) return 0.0;

       // Split at decimal point to find degree/minute boundary
       // Latitude:  DDMM.MMMM  → 2 degree digits
       // Longitude: DDDMM.MMMM → 3 degree digits
       bool ok = false;
       double raw = nmea.toDouble(&ok);
       if (!ok) return 0.0;

       int degrees = static_cast<int>(raw / 100);
       double minutes = raw - (degrees * 100.0);
       double decimal = degrees + (minutes / 60.0);

       if (hemisphere == "S" || hemisphere == "W")
           decimal = -decimal;

       return decimal;
   }

Sentence Priority for This Application
----------------------------------------

For each frame, we want the **freshest** available fix. The parsing strategy:

1. Parse every incoming sentence — GGA and RMC primarily
2. On every valid GGA: update position, altitude, fix quality, satellite count
3. On every valid RMC: update speed, track, and the **full date** (GGA has no date)
4. Combine into a single ``GnssRecord`` with the latest values from both
5. When a frame is captured, snapshot the current ``GnssRecord`` atomically

This gives the most complete and fresh data per frame regardless of which
sentences the receiver is outputting at any given moment.
