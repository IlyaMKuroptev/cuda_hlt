#!/usr/bin/python

# Converts the output from the PrPixelTrack debug mode,
# into my own debug format

import re
import sys
if len(sys.argv) < 3:
    print "Usage: comparetracks.py <prpixel-outputlog> <gpupixel-outputlog> [--inverse]"


# Finds a track, defined by the first hit's hitid
def findTrack(trackSearched, tracks):
    try:
        hitid = trackSearched['hits'][0]['hitid']
        for track in tracks:
            if hitid == track['hits'][0]['hitid']:
                return track
        return None
    except:
        print trackSearched
        raise

# Compares two lists of tracks
def compareTracks(tracks_list1, tracks_list2):
    tracks_list = []
    for track in tracks_list1:
        if not track in tracks_list:
            tracks_list.append(track)
            compareTrack(track, tracks_list2)
            print
        else:
            print "Track ", track['hits'][0]['hitid'], "appears repeated in the first list of tracks"

# Compares two tracks
def compareTrack(trackA, tracks):
    trackB = findTrack(trackA, tracks)
    if trackB == None:
        print "Track ID", trackA['hits'][0]['hitid'], "has no corresponding track"
        for hitinfo in trackA['hits']:
            print " -", hitinfo['hitid'], "module", hitinfo['module'], "x", hitinfo['x'], "y", hitinfo['y'], "z", hitinfo['z']
    elif trackA['nhits'] != trackB['nhits']:
        print "Tracks with ID", trackA['hits'][0]['hitid'], "differ:"
        print " nohits:", trackA['nhits'], "vs", trackB['nhits']
        print " hit differences:"
        # Compare hits
        hitIDsA = [a['hitid'] for a in trackA['hits']]
        hitIDsB = [a['hitid'] for a in trackB['hits']]
        # Get the list of hits equal, prefix " "
        equal = [a for a in hitIDsA if a in hitIDsB]
        for hitid in equal:
            hitinfo = [a for a in trackA['hits'] if a['hitid']==hitid][0]
            print "  ", hitinfo['hitid'], "module", hitinfo['module'], "x", hitinfo['x'], "y", hitinfo['y'], "z", hitinfo['z']
        # Get the list of hits missing in trackB, prefix "-"
        bmissing = [a for a in hitIDsA if a not in hitIDsB]
        for hitid in bmissing:
            hitinfo = [a for a in trackA['hits'] if a['hitid']==hitid][0]
            print " -", hitinfo['hitid'], "module", hitinfo['module'], "x", hitinfo['x'], "y", hitinfo['y'], "z", hitinfo['z']
        # Get the list of hits missing in trackA, prefix "+"
        amissing = [a for a in hitIDsB if a not in hitIDsA]
        for hitid in amissing:
            hitinfo = [a for a in trackB['hits'] if a['hitid']==hitid][0]
            print " +", hitinfo['hitid'], "module", hitinfo['module'], "x", hitinfo['x'], "y", hitinfo['y'], "z", hitinfo['z']
    else:
        print "Track ID", trackA['hits'][0]['hitid'], "are equal! :)"


def read_prpixel_file(filename):
    f = open(filename)
    s = ''.join(f.readlines())
    f.close()

    # Track:
    # {'ntrack':, 'nhits':, 'hits:' [{'hitid':, 'module':, 'x':, 'y':, 'z':}, ...]}
    prpixel_tracks = []

    # Find all debug lines with created tracks
    for i in re.finditer('Store track Nb (?P<ntrack>\d+)[^\d]nhits (?P<nhits>\d+).*?PrPixelTracking[ \t]*?(INFO ===|DEBUG)', s, re.DOTALL):
        hits = []
        # Find all hits in the track
        for j in re.finditer('PrPixelTracking.*?(?P<hitid>\d+) *module *(?P<module>\d+) x *(?P<x>[\d\.\-]+) y *(?P<y>[\d\.\-]+) z *(?P<z>[\d\-\.]+) used \d', i.group(0), re.DOTALL):
            hits.append({'hitid': j.group('hitid'),
                'module': j.group('module'),
                'x': j.group('x'),
                'y': j.group('y'),
                'z': j.group('z')})

        prpixel_tracks.append({'ntrack': i.group('ntrack'),
            'nhits': i.group('nhits'),
            'hits': hits})

    return prpixel_tracks


def read_gpupixel_file(filename):
    # The same for gpupixel
    f = open(filename)
    s = ''.join(f.readlines())
    f.close()

    gpupixel_tracks = []

    for i in re.finditer("Track #(?P<ntrack>\d+), length (?P<nhits>\d+)\n(?P<hits>.*?)\n\n", s, re.DOTALL):
        hits = []
        for j in re.finditer(" (?P<hitid>\d+) module *(?P<module>\d+), x *(?P<x>[\d\.\-]+), y *(?P<y>[\d\.\-]+), z *(?P<z>[\d\.\-]+)", i.group(0), re.DOTALL):
            hits.append({'hitid': j.group('hitid'),
                'module': j.group('module'),
                'x': j.group('x'),
                'y': j.group('y'),
                'z': j.group('z')})

        gpupixel_tracks.append({'ntrack': i.group('ntrack'),
            'nhits': i.group('nhits'),
            'hits': hits})

    return gpupixel_tracks


def readfile(filename, type):
    if type == "prpixel":
        return read_prpixel_file(filename)
    else:
        return read_gpupixel_file(filename)

# Print per track the compareTrack info and a space
def main():
    prpixel_filename = sys.argv[1]
    gpupixel_filename = sys.argv[2]

    # I know, this is just a hack...
    inverse = False
    try: inverse = sys.argv[3]
    except: pass

    prpixel_tracks = readfile(prpixel_filename, "prpixel")
    gpupixel_tracks = readfile(gpupixel_filename, "gpupixel")

    if inverse == "--inverse":
        compareTracks(gpupixel_tracks, prpixel_tracks)
    else:
        compareTracks(prpixel_tracks, gpupixel_tracks)

main()
