var data;
var chartRdy = false;
var tick = 0;
var dontDraw = false;
var halt = false;
var chartHeigth = 500;
var simValue1 = 0;
var simValue2 = 0;
var table;
var presc = 1;
var simMssgCnts = 0;
var lastTimeStamp = 0;
var REQINTERVAL = 30; // sec
var firstRequest = true;
var plotTimer = 6; // every 60 seconds plot averaged value
var rows = 0;

var MINUTESPERTICK = 5;// log interval 
var LOGDAYS = 1;
var MAXPOINTS = LOGDAYS * 24 * 60 / MINUTESPERTICK;

var dayNames = ['zo', 'ma', 'di', 'wo', 'do', 'vr', 'za'];
var displayNames = ["", "1", "2", "3", "4"];
var cbIDs =["","T1cb", "T2cb", "T3cb", "T4cb"]; 
var chartSeries = [-1,-1,-1,-1];

var NRItems = displayNames.length;


var chartOptions = {
	title: '',
	curveType: 'function',
	legend: { position: 'bottom' },

	heigth: 200,
	crosshair: { trigger: 'both' },	// Display crosshairs on focus and selection.
	explorer: {
		actions: ['dragToZoom', 'rightClickToReset'],
		//actions: ['dragToPan', 'rightClickToReset'],
		axis: 'horizontal',
		keepInBounds: true,
		maxZoomIn: 100.0
	},
	chartArea: { 'width': '90%', 'height': '60%' },

	vAxes: {
		0: { logScale: false },
	},
/*	series: {
		0: { targetAxisIndex: 0 },// T1
		1: { targetAxisIndex: 0 },// T2
		2: { targetAxisIndex: 0 },// T3
		3: { targetAxisIndex: 0 },// T4
		4: { targetAxisIndex: 0 },// Tref
	},*/
};

function clear() {
	data.removeRows(0, data.getNumberOfRows());
	chart.draw(data, options);
	tick = 0;
}

//var formatter_time= new google.visualization.DateFormat({formatType: 'long'});
// channel 1 .. 5

function plot(channel, value) {
	if (chartRdy) {
		if (channel == 1) {
			data.addRow();
			if (data.getNumberOfRows() > MAXPOINTS == true)
				data.removeRows(0, data.getNumberOfRows() - MAXPOINTS);
		}
		if (value != '--'){
			if ( value > -50.0) {
				value = parseFloat(value); // from string to float
				data.setValue(data.getNumberOfRows() - 1, channel, value);
			}
		}
	}
}

function loadCBs (){
   var cbstate;
   
   console.log( "Reading CBs");
   
	 // Get the current state from localstorage
    // State is stored as a JSON string
    cbstate = JSON.parse(localStorage['CBState'] || '{}');
  
    // Loop through state array and restore checked 
    // state for matching elements
    for(var i in cbstate) {
      var el = document.querySelector('input[name="' + i + '"]');
      if (el) el.checked = true;
    }
  
    // Get all checkboxes that you want to monitor state for
    var cb = document.getElementsByClassName('save-cb-state');
  
    // Loop through results and ...
    for(var i = 0; i < cb.length; i++) {
  
      //bind click event handler
      cb[i].addEventListener('click', function(evt) {
        // If checkboxe is checked then save to state
        if (this.checked) {
          cbstate[this.name] = true;
        }
    
    // Else remove from state
        else if (cbstate[this.name]) {
          delete cbstate[this.name];
        }
    
    // Persist state
        localStorage.CBState = JSON.stringify(cbstate);
      });
    }
    console.log( "CBs read" );
    initTimer();
  };
	

function initChart() {
	window.addEventListener('load', loadCBs() );
}

function initTimer() {
	var activeSeries = 1;
	chart = new google.visualization.LineChart(document.getElementById('temperaturesChart'));
	data = new google.visualization.DataTable();
	data.addColumn('string', 'Time');
	
	for (var m = 1; m < NRItems; m++) { // time not used for now 
		var cb = document.getElementById(cbIDs[m]);
		if (cb) {
			if (cb.checked) {
				data.addColumn('number', displayNames[m]);
				chartSeries[m] = activeSeries;
				activeSeries++;
			}
		}
	}
	chartRdy = true;
	if (SIMULATE) {
		simplot();
	}
	else {
		setInterval(function() { timer() }, 1000);
	}
}


function updateLastDayTimeLabel(data) {
	var ms = Date.now();
	var date = new Date(ms);
	var labelText = date.getHours() + ':' + date.getMinutes();
	data.setValue(data.getNumberOfRows() - 1, 0, labelText);
}

function updateAllDayTimeLabels(data) {
	var rows = data.getNumberOfRows();
	var minutesAgo = rows * MINUTESPERTICK;
	var ms = Date.now();
	ms -= (minutesAgo * 60 * 1000);
	for (var n = 0; n < rows; n++) {
		var date = new Date(ms);
		var labelText = dayNames[date.getDay()] + ';' + date.getHours() + ':' + date.getMinutes();
		data.setValue(n, 0, labelText);
		ms += 60 * 1000 * MINUTESPERTICK;

	}
}

function simplot() {
	simValue1 += 0.001;
	simValue2 = Math.sin(simValue1) * 0.0001;
	var str = "0,1,2,3,4,5,\n";
	var str2;
	for (var n = 0; n < 20; n++)
		str2 = str + str2;
	plotArray(str2);

	for (var m = 1; m < NRItems; m++) { // time not used for now 
		var value = simValue2; // from string to float
		document.getElementById(displayNames[m]).innerHTML = value.toFixed(2);
	}
}

function plotArray(str) {
	var arr;
	var arr2 = str.split("\n");
	var nrPoints = arr2.length - 1;

	for (var p = 0; p < nrPoints; p++) {
		arr = arr2[p].split(",");
		if (arr.length >= NRItems) {
			for (var m = 1; m < NRItems; m++) { // time not used for now
			if (	chartSeries[m] != -1 )   
				plot(chartSeries[m] , arr[m]); 
			}
		}
	}
	if (nrPoints == 1) { // then single point added 
		updateLastDayTimeLabel(data);
	}
	else {
		updateAllDayTimeLabels(data);
	}
	chart.draw(data, chartOptions);
}

function timer() {
	var arr;
	var str;
	presc--
	if (SIMULATE) {
		simplot();
	}
	else {
		if (presc == 0) {
			presc = REQINTERVAL;
			str = getRTMeasValues();
			arr = str.split(",");
			// print RT values 
			if (arr.length >= NRItems) {
				if (arr[0] > 0) {
					if (arr[0] != lastTimeStamp) {
						lastTimeStamp = arr[0];
						for (var m = 1; m < NRItems; m++) { // time not used for now 
							var value = parseFloat(arr[m]); // from string to float
							if (value < -100)
								arr[m] = "--";
							document.getElementById(displayNames[m]).innerHTML = arr[m];
							if (chartSeries[m] != -1 )   
								plot(chartSeries[m] , arr[m]);
						}
						updateLastDayTimeLabel(data);
						chart.draw(data, chartOptions);
					}
				}
			}
			if (firstRequest) {
				arr = getLogMeasValues();
				plotArray(arr);
				firstRequest = false;
			}
		}
	}
}




