#!/usr/bin/python

graceTemplate = """
@version 50121
@timestamp on
@reference date 2440587.5
@default sformat "%%.24g"
@with string
@    string on
@    string loctype view
@    string 0.6, 0.03
@    string color 1
@    string rot 0
@    string font 0
@    string just 0
@    string char size 1.000000
@    string def "%(plotName)s"

@g0 on
@g0 fixedpoint on
@g0 fixedpoint type 1
@with g0
@    view 0.100000, 0.250000, 1.200000, 0.850000
@    title "%(title)s"
@    title size 1.20
@    subtitle "%(subtitle)s"
@    xaxes scale Normal
@    yaxes scale Normal
@    xaxes invert off
@    yaxes invert off
@    xaxis  on
@    xaxis  type zero false
@    xaxis  offset 0.000000 , 0.000000
@    xaxis  bar on
@    xaxis  bar color 1
@    xaxis  bar linestyle 1
@    xaxis  bar linewidth 1.0
@    xaxis  label "turnaround time [ms]"
@    xaxis  label layout para
@    xaxis  label place auto
@    xaxis  label char size 1.000000
@    xaxis  label font 21
@    xaxis  label color 1
@    xaxis  label place normal
@    xaxis  tick on
#@    xaxis  tick major 10000
@    xaxis  tick minor ticks 9
@    xaxis  tick default 4
@    xaxis  tick place rounded false
@    xaxis  tick in
@    xaxis  tick major size 0.000000
@    xaxis  tick major color 1
@    xaxis  tick major linewidth 1.0
@    xaxis  tick major linestyle 3
@    xaxis  tick major grid on
@    xaxis  tick minor color 7
@    xaxis  tick minor linewidth 1.0
@    xaxis  tick minor linestyle 3
@    xaxis  tick minor grid off
@    xaxis  tick minor size 0.000000
@    xaxis  ticklabel on
@    xaxis  ticklabel format decimal
@    xaxis  ticklabel prec 0
@    xaxis  ticklabel formula ""
@    xaxis  ticklabel append ""
@    xaxis  ticklabel prepend ""
@    xaxis  ticklabel angle 91
@    xaxis  ticklabel skip 0
@    xaxis  ticklabel stagger 0
@    xaxis  ticklabel place normal
@    xaxis  ticklabel offset auto
@    xaxis  ticklabel offset 0.000000 , 0.010000
@    xaxis  ticklabel start type auto
@    xaxis  ticklabel start 0.000000
@    xaxis  ticklabel stop type auto
@    xaxis  ticklabel stop 0.000000
@    xaxis  ticklabel char size 0.680000
@    xaxis  ticklabel font 21
@    xaxis  ticklabel color 1
@    xaxis  tick place both
@    xaxis  tick spec type none
@    yaxis  on
@    yaxis  type zero false
@    yaxis  offset 0.000000 , 0.000000
@    yaxis  bar on
@    yaxis  bar color 1
@    yaxis  bar linestyle 1
@    yaxis  bar linewidth 1.0
@    yaxis  label "requests"
@    yaxis  label layout para
@    yaxis  label place auto
@    yaxis  label char size 1.000000
@    yaxis  label font 21
@    yaxis  label color 1
@    yaxis  label place normal
@    yaxis  tick on
#@    yaxis  tick major 50
@    yaxis  tick minor ticks 1
@    yaxis  tick default 12
@    yaxis  tick place rounded true
@    yaxis  tick in
@    yaxis  tick major size 1.000000
@    yaxis  tick major color 1
@    yaxis  tick major linewidth 1.0
@    yaxis  tick major linestyle 3
@    yaxis  tick major grid on
@    yaxis  tick minor color 7
@    yaxis  tick minor linewidth 1.0
@    yaxis  tick minor linestyle 3
@    yaxis  tick minor grid on
@    yaxis  tick minor size 0.500000
@    yaxis  ticklabel on
@    yaxis  ticklabel on
@    yaxis  ticklabel format decimal
@    yaxis  ticklabel prec 0
@    yaxis  ticklabel formula ""
@    yaxis  ticklabel append ""
@    yaxis  ticklabel prepend ""
@    yaxis  ticklabel angle 0
@    yaxis  ticklabel skip 0
@    yaxis  ticklabel stagger 0
@    yaxis  ticklabel place normal
@    yaxis  ticklabel offset auto
@    yaxis  ticklabel offset 0.000000 , 0.010000
@    yaxis  ticklabel start type auto
@    yaxis  ticklabel start 0.000000
@    yaxis  ticklabel stop type auto
@    yaxis  ticklabel stop 0.000000
@    yaxis  ticklabel char size 0.680000
@    yaxis  ticklabel font 21
@    yaxis  ticklabel color 1
@    yaxis  tick place both
@    yaxis  tick spec type none
@    altxaxis  off
@    altyaxis  off
@    legend on
@    legend loctype view
@    legend 0.15, 0.97
@    legend vgap 0
@    legend char size 1.00000

@with g0
%(setsData)s

@autoscale
@autoticks
#@page out
@redraw
"""

graceSet ="""
@target s%(setNo)d
%(data)s
&
@sort s_ X ASCENDING
@s%(setNo)d type xy
@s%(setNo)d line type %(linetype)d
@s%(setNo)d line color %(color)d
@s%(setNo)d symbol 0
@s%(setNo)d symbol size %(barsize)d
@s%(setNo)d symbol color %(color)d
@s%(setNo)d symbol pattern 1
@s%(setNo)d symbol fill color %(color)d
@s%(setNo)d symbol fill pattern 0
@s%(setNo)d symbol linewidth 1.0
@s%(setNo)d symbol linestyle 1
@s%(setNo)d symbol char 65
@s%(setNo)d symbol char font 0
@s%(setNo)d symbol skip 1
@s%(setNo)d comment "%(legend)s"
#@s%(setNo)d legend "%(legend)s"
"""

def generate(stream, options):
	boxes = {}
	maxbox = 0
	sum = 0.0
	numRes = 0
	tats = []
	for line in stream.readlines():
		if line.startswith('start '):
			startTime = float(line[6:]) / (10 ** 9) # sec prec
			continue
		if line.startswith('stop  '):
			stopTime = float(line[6:]) / (10 ** 9) # sec prec
			continue
		tat = float(line) / 1000 # usec precision
		tats.append(tat)
		#print tat
		sum += tat
		numRes += 1
		box = int (tat / options.boxSize) # each box spans for BOX_SIZE usec
		boxes[box] = boxes.get(box, 0) + 1
		if box > maxbox:
			maxbox = box
	
	tats.sort()
	perc90 = tats[int(len(tats) * 0.9)] / 1000
	perc99 = tats[int(len(tats) * 0.99)] / 1000
	assert numRes == options.clientThreads * options.queriesPerClient
	setData = []
	for box in range(0, maxbox + 1):
		setData.append("%.3f %d" % (float(box * options.boxSize) / 1000, boxes.get(box, 0)))
	data = "\n".join(setData)
	
	avgTat = sum / 1000 / numRes
	bw = numRes / (stopTime - startTime)
	setDesc = { 'setNo' : 0, 'color' : 1, 'data' : data, 'legend' : 'boxSize=%d, q=%d (%d), wt=%d, st=%s, bw=%.2f r/s, avgTat=%.2f ms,  90thTat=%.2f ms, 99thTath=%.2f ms' % 
		(options.boxSize, options.clientThreads * options.queriesPerClient, options.clientThreads, options.workerThreads, options.strategy, bw, avgTat, perc90, perc99),
		'barsize': (float(options.boxSize) / 1000), 'linetype': 1
	}
	
	allSets = graceSet % setDesc
	
	title = "queries=%d, clients=%d, workers=%d, strategy=%s, pending=%d\\nbandwidth=%.2f r/s, avgTat=%.2f ms,  90thTat=%.2f ms, 99thTath=%.2f ms)" % \
		(options.clientThreads * options.queriesPerClient, options.clientThreads, options.workerThreads, options.strategy, options.pending, bw, avgTat, perc90, perc99)
	
	program = graceTemplate % { 'setsData' : allSets, 'plotName' : 'distribution with boxSize = %d' % options.boxSize, 'title': title, 'subtitle': '' }
	
	return (program, bw, avgTat, perc90, perc99)
