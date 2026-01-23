# Hakuna Fucking Matata
## EuroScope Plugin

Simple plugin to "assume" control of an ACFT while staffing any ground station.

### Known issues:
1. Cannot assume GND ctrl of inbound / arriving ACFT if not using EuroScope's built-in arrival list du to limitations of the TopSky Plugin and GRP (GroundRadar Plugin)
2. Potential (insignificant) memory leak when ACFT goes out-of-scope while still being assumed
