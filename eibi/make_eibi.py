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
    return station

def has_number(word):
    for i in "0123456789":
        if i in word:
            return True
    return False

transmitter_memo = {}
def get_transmitter(ITU, transmitter_code):
    if (ITU, transmitter_code) in transmitter_memo:
        return transmitter_memo[(ITU, transmitter_code)]
    with open("README.TXT", encoding='utf-8', errors='replace') as inf:
        country_found = False

        #if transmitter is in a different country
        if transmitter_code.startswith("/"):
            if "-" not in transmitter_code:
                ITU = transmitter_code[1:]
                transmitter_code = ""
            else:
                ITU = transmitter_code.split("-")[0][1:]
                transmitter_code = transmitter_code.split("-")[1]

        for lineno, line in enumerate(inf):
            if lineno >= first_transmitter_line and lineno >= last_transmitter_line and line.strip().startswith(ITU+":"):
                country_found = True
                line = line.strip()[len(ITU)+1:]

            if country_found:
                line = line.strip()
                if line.startswith(transmitter_code+"-") or transmitter_code=="":

                    if "one of" in line: #hard to handle multiple possible locations
                        line = line[line.index(":")+1:line.index("/")].strip()
                    line=line.replace("- ", "-") #workaround for stray space in file
                    line=line.replace("except:", "") #workaround for comment

                    transmitter_name = line.strip()
                    #if transmitter code exists trim it off
                    if "-" in line.split()[0]:
                        transmitter_name = transmitter_name[transmitter_name.index("-")+1:].strip()

                    #Try to extract location if it is present
                    last_word = transmitter_name.split()[-1]
                    if "-" in last_word and (
                            "E" in last_word or
                            "W" in last_word or
                            "N" in last_word or
                            "S" in last_word) and has_number(last_word):
                        transmitter_name = " ".join(transmitter_name.split()[:-1])

                        lat,lon = last_word.split("-")
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
                    else:
                        lon, lat = 999, 999

                    #attempt to strip extra information on the same line
                    transmitter_name = transmitter_name.replace('"', '')
                    if "/" in transmitter_name:
                        transmitter_name = transmitter_name.split("/")[0]
                    if "," in transmitter_name:
                        transmitter_name = transmitter_name.split(",")[0]
                    if "(" in transmitter_name:
                        transmitter_name = transmitter_name.split("(")[0]
                    if ":" in transmitter_name:
                        transmitter_name = transmitter_name.split(":")[0]

                    transmitter_name = transmitter_name.strip()


                    transmitter_memo[(ITU, transmitter_code)] = (transmitter_name, (lat, lon))
                    return (transmitter_name, (lat, lon))

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

with open("reconstructed.txt", "w") as outf:
    for f, s, c, l, t, fr, to, dy in compressed:
        print(transmitter_locations[t][0])
        outf.write("%10u %21s %21s %21s %21s %i %i\n"%(
            f,
            list(stations.keys())[s],
            list(countries.keys())[c],
            list(languages.keys())[l],
            list(transmitter_names.keys())[t],
            transmitter_locations[t][0],
            transmitter_locations[t][1],
        ))



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
    outf.write('#include <cmath>\n')
    outf.write(station_array)
    outf.write(country_array)
    outf.write(language_array)
    outf.write(transmitter_array)
    outf.write(location_array)
    outf.write(frequency_array)
    outf.write("""
int16_t lookup_frequency(uint16_t frequency, int16_t &from, int16_t &to) {

  int left = 0;
  int right = NUM_FREQUENCIES-1;

  while (left <= right) {
    int mid = left + (right - left) / 2;
    if (frequencies[mid].frequency == frequency) {
      from = mid;
      while(from-1 > 0 && frequencies[from-1].frequency==frequency) from--;
      to = mid;
      while(to+1 < NUM_FREQUENCIES && frequencies[to+1].frequency==frequency) to++;
      return mid; // found
    } else if (frequencies[mid].frequency < frequency) {
      left = mid + 1; // search right half
    } else {
      right = mid - 1; // search left half
    }
  }

  return -1;
}

static float deg2rad(float d) { return d * M_PI / 180.0; }
double distance_km(float lon_a, float lat_a, float lon_b, float lat_b) {
    const double R = 6371.0; // Earth radius in km

    double lat1 = deg2rad(lat_a);
    double lat2 = deg2rad(lat_b);
    double dlat = lat2 - lat1;
    double dlon = deg2rad(lon_b - lon_a);

    double h = sin(dlat/2)*sin(dlat/2) +
               cos(lat1)*cos(lat2)*sin(dlon/2)*sin(dlon/2);

    return 2 * R * atan2(sqrt(h), sqrt(1 - h));
}""")


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

int16_t lookup_frequency(uint16_t frequency, int16_t &from, int16_t &to);
double distance_km(float lon_a, float lat_a, float lon_b, float lat_b);


#endif
"""%(len(compressed), len(stations), len(countries), len(languages), len(transmitter_names))
with open("eibi.h", "w") as outf:
    outf.write(header)

