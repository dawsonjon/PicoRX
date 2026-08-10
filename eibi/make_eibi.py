def get_first_occurance(word):
    with open("README.TXT", encoding='utf-8', errors='ignore') as inf:
        scanning = False
        for lineno, line in enumerate(inf):
            if "Aruba" in line:
                return lineno

    return None
first_country_line = get_first_occurance("Aruba")
last_country_line = get_first_occurance("Zimbabwe")
first_transmitter_line = get_first_occurance("AFG:")
last_transmitter_line = get_first_occurance("ZWE:")

def shorten(station):
    for i, j in [("Radio", "R."), ("Numbers", "Nrs."), ("Meteo Fax", "Wefax"), ("Met Fax", "Wefax")]:
        station = station.replace(i, j)

    if "(" in station:
        station = station.split("(")[0]

    if "," in station:
        station = station.split(",")[0]

    if len(station) > 21:
        for i in "aeiou":
            station = station.replace(i, "")
            if len(station) <= 21:
                break

    return station

transmitter_memo = {}
def get_transmitter(ITU, transmitter_code):
    if (ITU, transmitter_code) in transmitter_memo:
        return transmitter_memo[(ITU, transmitter_code)]
    with open("README.TXT", encoding='utf-8', errors='ignore') as inf:
        country_found = False

        #if transmitter is in a different country
        if transmitter_code.startswith("/"):
            if "-" not in transmitter_code:
                ITU = transmitter_code[1:]
                transmitter_code = "any"
            else:
                ITU = transmitter_code.split("-")[0][1:]
                transmitter_code = transmitter_code.split("-")[1]

        for lineno, line in enumerate(inf):
            if lineno >= first_transmitter_line and lineno >= last_transmitter_line and line.strip().startswith(ITU+":"):
                country_found = True
                line = line.strip()[len(ITU)+1:]

            if country_found:
                line = line.strip()
                if line.startswith(transmitter_code+"-") or transmitter_code=="any":
                    if "one of" in line:
                        return ("", (999, 999))

                    line=line.replace("- ", "-")
                    line=line.replace("except:", "")
                    if "-" in line.split()[0]:
                        transmitter_name = " ".join(line.split()[:-1])[len(transmitter_code)+1:].strip()
                    else:
                        transmitter_name = " ".join(line.split()[:-1]).strip()

                    transmitter_name = transmitter_name.replace('"', '')
                    if "/" in transmitter_name:
                        transmitter_name = transmitter_name.split("/")[0]
                    if "," in transmitter_name:
                        transmitter_name = transmitter_name.split(",")[0]
                    if "(" in transmitter_name:
                        transmitter_name = transmitter_name.split("(")[0]
                    if ":" in transmitter_name:
                        transmitter_name = transmitter_name.split(":")[0]
                    while len(transmitter_name) > 21:
                        transmitter_name = " ".join(transmitter_name.split()[:-1])

                    location = line.split()[-1]
                    try:
                        lat,lon=location.split("-")
                        if "'" in lon:
                            lon = lon.split("'")[0]
                        if "'" in lat:
                            lat = lat.split("'")[0]
                        if "N" in lon or "S" in lon:
                            lat, lon = lon, lat

                        if "E" in lon:
                            lon = int(lon.split("E")[0])
                        else:
                            lon = -int(lon.split("W")[0])
                        if "N" in lat:
                            lat = int(lat.split("N")[0])
                        else:
                            lat = -int(lat.split("S")[0])
                    except ValueError:
                        lon, lat = 999, 999
                    transmitter_memo[(ITU, transmitter_code)] = (transmitter_name, (lat, lon))
                    return (transmitter_name, (lat, lon))

                if ":" in line:
                    return ("", (999, 999))

    return ("", (999, 999))

country_memo = {}
def get_country(ITU):
    if ITU in country_memo:
        return country_memo[ITU]
    with open("README.TXT", encoding='utf-8', errors='ignore') as inf:
        for lineno, line in enumerate(inf):
            if lineno >= first_country_line and lineno >= last_country_line and line.split()[0]==ITU:
                country = " ".join(line.split()[1:])
                if "(" in country:
                    country = country.split("(")[0]
                if "," in country:
                    country = country.split(",")[0]
                if "*" in country:
                    country = country.split("*")[0]
                if country.startswith("United Kingdom of"):
                    country = "United Kingdom"
                if country.startswith("United States of"):
                    country = "United States"
                country = country.strip()
                country_memo[ITU] = country
                return country
    return ""

language_memo = {}
def get_language(language_code):
    if language_code in language_memo:
        return language_memo[language_code]
    with open("README.TXT", encoding='utf-8', errors='ignore') as inf:
        for line in inf:
            if line.strip().split() and line.strip().split()[0]==language_code and line.strip().endswith("]"):
                language = line.strip().split()[1].strip(":")
                if("/" in language):
                    language=language.split("/")[0]
                language_memo[language_code] = language
                return language
    return ""

def parse_database():
    data = []
    with open("sked-a26.csv", encoding='utf-8', errors='ignore') as inf:
        for line in inf:
            fields = line.split(";")
            data.append(fields)

    compressed = []
    stations = {}
    countries = {}
    languages = {}
    transmitter_names = {}
    transmitter_locations = []
    count = 0
    country_count = 0
    language_count = 0
    transmitter_count = 0
    for frequency, time, days, ITU, station, lng, Target, Remarks, P, Start, Stop in data[1:]:
        frequency = int(round(float(frequency)))

        _from, _to = time.split("-")
        _from = int(_from[:2])*60+int(_from[2:])
        _to = int(_to[:2])*60+int(_to[2:])

        day_lookup = {"Mo":1,"Tu":2,"We":3,"Th":4,"Fr":5,"Sa":6,"Su":0}
        day_flags = []
        if "-" in days:
            a, b = days.split("-")
            a = day_lookup[a]
            b = day_lookup[b]
            if(a > b):
                b+=7
            day_flags = [i%7 for i in range(a, b+1)]
        elif "," in days:
            day_flags = [day_lookup[i] for i in days.split(",")]
        elif days.strip() in day_lookup:
            day_flags = [day_lookup[days.strip()]]
        elif days.strip() == "":
            day_flags = list(range(8))

        day_int = sum([1<<i for i in day_flags])

        station = shorten(station)
        if station in stations:
            station_id = stations[station]
        else:
            station_id = count
            count+=1
            stations[station] = station_id

        transmitter_name, transmitter_location = get_transmitter(ITU, Remarks)
        if transmitter_name in transmitter_names:
            transmitter_id = transmitter_names[transmitter_name]
        else:
            transmitter_id = transmitter_count
            transmitter_count+=1
            transmitter_names[transmitter_name]=transmitter_id
            transmitter_locations.append(transmitter_location)

        country = get_country(ITU)
        if country in countries:
            country_id = countries[country]
        else:
            country_id = country_count
            country_count+=1
            countries[country] = country_id

        language = get_language(lng)
        if language in languages:
            language_id = languages[language]
        else:
            language_id = language_count
            language_count+=1
            languages[language] = language_id

        if((frequency, station_id, country_id, language_id)) not in compressed[:5]:
            compressed.append((frequency, station_id, country_id, language_id, transmitter_id, _from, _to, day_int))

    size = 0
    for station in stations:
        size += len(station)

    country_size = 0
    for country in countries:
        country_size += len(country)

    language_size = 0
    for language in languages:
        language_size += len(language)

    transmitter_names_size = 0
    for transmitter_name in transmitter_names:
        transmitter_names_size += len(transmitter_name)

    print(len(stations), "Stations (kB)", size/1024)
    print(len(countries), "Countries (kB)", country_size/1024)
    print(len(languages), "Languages (kB)", language_size/1024)
    print(len(transmitter_names), "Transmitter Names (kB)", transmitter_names_size/1024)
    print("Frequencies (kB)", 9*len(compressed)/1024)

    return compressed, stations, countries, languages, transmitter_names, transmitter_locations

compressed, stations, countries, languages, transmitter_names, transmitter_locations = parse_database()

station_array = ",\n".join(['"%s"'%i for i in stations])
station_array = "const char* const stations[] = {\n%s};\n"%station_array
country_array = ",\n".join(['"%s"'%i for i in countries])
country_array = "const char* const countries[] = {\n%s};\n"%country_array
language_array = ",\n".join(['"%s"'%i for i in languages])
language_array = "const char* const languages[] = {\n%s};\n"%language_array
transmitter_array = ",\n".join(['"%s"'%i for i in transmitter_names])
transmitter_array = "const char* const transmitters[] = {\n%s};\n"%transmitter_array
location_array = ",\n".join(['{%i, %i}'%(i, j) for i, j in transmitter_locations])
location_array = "const s_locations locations[] = {\n%s};\n"%location_array
frequency_array = ",\n".join(['{%i, %i, %i, %i, %i, %i, %i, %i}'%(f, s, c, l, t, fr, to, dy) for f, s, c, l, t, fr, to, dy in compressed])
frequency_array = "const s_frequency frequencies[] = {\n%s};\n"%frequency_array
with open("eibi.cpp", "w") as outf:
    outf.write('#include "eibi.h"\n')
    outf.write(station_array)
    outf.write(country_array)
    outf.write(language_array)
    outf.write(transmitter_array)
    outf.write(location_array)
    outf.write(frequency_array)

header = """

#ifndef __EIBI_H__
#define __EIBI_H__
#include <cstdint>

struct s_locations{
    int16_t  lat;
    int16_t  lon;
};

struct s_frequency{
    uint16_t frequency;
    uint16_t station_id;
    uint8_t  country_id;
    uint16_t language_id;
    uint16_t transmitter_id;
    uint16_t from;
    uint16_t to;
    uint8_t dayflags;
};

static const uint16_t NUM_FREQUENCIES=%i;
static const uint16_t NUM_STATIONS=%i;
static const uint16_t NUM_COUNTRIES=%i;
static const uint16_t NUM_LANGUAGES=%i;
static const uint16_t NUM_TRANSMITTERS=%i;

extern const char* const stations[NUM_STATIONS];
extern const char* const countries[NUM_COUNTRIES];
extern const char* const languages[NUM_LANGUAGES];
extern const char* const transmitters[NUM_TRANSMITTERS];
extern const s_frequency frequencies[NUM_FREQUENCIES];
extern const s_locations locations[NUM_TRANSMITTERS];


#endif
"""%(len(compressed), len(stations), len(countries), len(languages), len(transmitter_names))
with open("eibi.h", "w") as outf:
    outf.write(header)

