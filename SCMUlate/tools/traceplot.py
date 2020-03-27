#!/usr/bin/env python3

import plotly.graph_objects as go
from enum import Enum
import argparse
import json
import sys

''' List of possible colors
aliceblue, antiquewhite, aqua, aquamarine, azure,
beige, bisque, black, blanchedalmond, blue,
blueviolet, brown, burlywood, cadetblue,
chartreuse, chocolate, coral, cornflowerblue,
cornsilk, crimson, cyan, darkblue, darkcyan,
darkgoldenrod, darkgray, darkgrey, darkgreen,
darkkhaki, darkmagenta, darkolivegreen, darkorange,
darkorchid, darkred, darksalmon, darkseagreen,
darkslateblue, darkslategray, darkslategrey,
darkturquoise, darkviolet, deeppink, deepskyblue,
dimgray, dimgrey, dodgerblue, firebrick,
floralwhite, forestgreen, fuchsia, gainsboro,
ghostwhite, gold, goldenrod, gray, grey, green,
greenyellow, honeydew, hotpink, indianred, indigo,
ivory, khaki, lavender, lavenderblush, lawngreen,
lemonchiffon, lightblue, lightcoral, lightcyan,
lightgoldenrodyellow, lightgray, lightgrey,
lightgreen, lightpink, lightsalmon, lightseagreen,
lightskyblue, lightslategray, lightslategrey,
lightsteelblue, lightyellow, lime, limegreen,
linen, magenta, maroon, mediumaquamarine,
mediumblue, mediumorchid, mediumpurple,
mediumseagreen, mediumslateblue, mediumspringgreen,
mediumturquoise, mediumvioletred, midnightblue,
mintcream, mistyrose, moccasin, navajowhite, navy,
oldlace, olive, olivedrab, orange, orangered,
orchid, palegoldenrod, palegreen, paleturquoise,
palevioletred, papayawhip, peachpuff, peru, pink,
plum, powderblue, purple, red, rosybrown,
royalblue, saddlebrown, salmon, sandybrown,
seagreen, seashell, sienna, silver, skyblue,
slateblue, slategray, slategrey, snow, springgreen,
steelblue, tan, teal, thistle, tomato, turquoise,
violet, wheat, white, whitesmoke, yellow,
yellowgreen
'''

color_map = {
    "SYS_TIMER": "lightslategray",
    "SU_TIMER": "peachpuff",
    "MEM_TIMER": "midnightblue",
    "CU_TIMER": "mediumseagreen",
    "SYS_START": "lightgray",
    "SYS_END": "yellow",
    "SU_START": "thistle",
    "SU_END": "firebrick",
    "FETCH_DECODE_INSTRUCTION": "mediumblue",
    "DISPATCH_INSTRUCTION": "blue",
    "EXECUTE_CONTROL_INSTRUCTION": "darkblue",
    "EXECUTE_ARITH_INSTRUCTION": "black",
    "SU_IDLE": "whitesmoke",
    "MEM_START": "silver",
    "MEM_END": "teal",
    "MEM_EXECUTION": "yellowgreen",
    "MEM_IDLE": "palevioletred",
    "CU_START": "mediumaquamarine",
    "CU_END": "linen",
    "CU_EXECUTION": "darkviolet",
    "CU_IDLE": "darkslateblue"
}

class counter_type(Enum):
    SYS_TIMER = 0
    SU_TIMER = 1
    MEM_TIMER = 2
    CU_TIMER = 3

class SYS_event(Enum):
    SYS_START = 0
    SYS_END = 1

class SU_event(Enum):
    SU_START = 0
    SU_END = 1
    FETCH_DECODE_INSTRUCTION = 2
    DISPATCH_INSTRUCTION = 3
    EXECUTE_CONTROL_INSTRUCTION = 4
    EXECUTE_ARITH_INSTRUCTION = 5
    SU_IDLE = 6

class MEM_event(Enum):
    MEM_START = 0
    MEM_END = 1
    MEM_EXECUTION = 2
    MEM_IDLE = 3

class CU_event(Enum):
    CU_START = 0
    CU_END = 1
    CU_EXECUTION = 2
    CU_IDLE = 3

def getEnumPerType(typeName, eventID):
    if typeName == "SYS_TIMER":
        return SYS_event(eventID)
    if typeName == "SU_TIMER":
        return SU_event(eventID)
    if typeName == "MEM_TIMER":
        return MEM_event(eventID)
    if typeName == "CU_TIMER":
        return CU_event(eventID)
    return None


def load_file(fileName):
    try:
        with open(fileName) as f:
            return json.load(f)
    except:
        print("Error opening file", file=sys.stderr)
        exit()

class traces:
    curNumPlots = 0
    plotNum = {}
    y = {}
    x = {}
    legend = {}
    bases = {}
    colors = {}
    fig = None
    def __init__(self):
        self.curNumPlots = 0
        self.curNumPlots = 0
        self.y = {}
        self.x = {}
        self.bases = {}
        self.colors = {}
        self.fig = go.Figure()

    
    def addNewTrace(self, name):
        if name in self.plotNum:
            print("Trying to add same plot twice", file=sys.stderr)
            exit()
        self.plotNum[name] = self.curNumPlots
        self.curNumPlots += 1

    def addNewPlot(self, name, start, end, eventType):
        if name not in self.y:
            self.y[name] = []
        if name not in self.x:
            self.x[name] = []
        if name not in self.bases:
            self.bases[name] = []
        if name not in self.legend:
            self.legend[name] = []
        if name not in self.colors:
            self.colors[name] = []
        if name not in self.plotNum:
            print("Plot does not exist", file=sys.stderr)
            exit()
        
        self.y[name].append(name)
        self.x[name].append(end)
        self.bases[name].append(start)
        self.legend[name].append(eventType + " [" + str(end) + " s]")
        self.colors[name].append(color_map[eventType])
    
    def plotTrace(self):
        for name, num in self.plotNum.items():
            if name in self.x:
                self.fig.add_trace(go.Bar(x=self.x[name], y=self.y[name], base=self.bases[name], hovertext=self.legend[name], orientation='h', marker_color=self.colors[name]))
            else:
                self.fig.add_trace(go.Bar(x=[0], y=[name], orientation='h'))
        self.fig.update_layout(barmode='relative', title_text='Relative Barmode', dragmode=False )
        self.fig.show()


    

def main():
    parser = argparse.ArgumentParser(description='Plots the trace result of SCMUlate')
    parser.add_argument('--input', '-i', dest='fileName', action='store', nargs=1,
                    help='File name to be plot', required=True)

    args = parser.parse_args()
    fileName = args.fileName[0]

    tracePloter = traces()
    if fileName != "":
        data = load_file(fileName)
        for name, counter in data.items():
            tracePloter.addNewTrace(name)
            prevTime = None
            prevType = ""
            counter_t = counter_type(int(counter["counter type"])).name
            for event in counter["events"]:
                if prevTime is None:
                    prevTime = event["value"]
                    prevType = event["type"]
                else:
                    typeEnum = getEnumPerType(counter_t, int(prevType))
                    if (typeEnum.name[-4:] != "IDLE" and typeEnum.name[-3:] != "END" and typeEnum.name[-5:] != "START"):
                        tracePloter.addNewPlot(name, prevTime, event["value"] - prevTime, typeEnum.name )
                    prevTime = event["value"]
                    prevType = event["type"]

        tracePloter.plotTrace()
    else:
        print("No file specified")
        parser.print_help()
        exit()

    

if __name__ == "__main__":
    main()