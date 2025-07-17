var Clay = require('pebble-clay');
var clayConfig = require('./config');
var customClay = require('./custom-clay');
var clay = new Clay(clayConfig, customClay);


Pebble.addEventListener("ready",
                        function(e) {
                            console.log("JavaScript app here!");
                            Pebble.sendAppMessage({'ack': true});
                            //                            Pebble.timelineSubscribe('changes',
                            //                                                     function () 
                            //                                                     {
                            //                                                         console.log('Subscribed to changes');
                            //                                                     },
                            //                                                     function (errorString) 
                            //                                                     {
                            //                                                         console.log('Error subscribing to changes: ' + errorString);
                            //                                                     }
                            //                                );

			    initialised = true;

			    var options = JSON.parse(localStorage.getItem('options'));
			    var options2 = JSON.parse(localStorage.getItem('clay-settings'));

			    console.log("New options: " + JSON.stringify(options2));

                            if (options2) {
			        console.log("Installing options: " + JSON.stringify(options2));
                                options2["changeit"] = false; /* init */
                                options2["ago"] = "0"; /* init */
                                localStorage.setItem('clay-settings', JSON.stringify(options2));
                                return; /* if already done with compat. stuff, then return now */
                            }

                            /*
                              var options2 = {
                              podchange: options["podchange"],
                              changeit: options["changeit"],
                              timer: options["timer"],
                              };
			      console.log("Continuing after options 2");
                              options2["displayMonth"] = false;
                              options2["displayDate"] = false;
                              options2["displayHour"] = false;
                              options2["displayMinute"] = false;
                              options2["displayBluetooth"] = false;
                              options2["updates"] = false;
                              if (options["month"] == "1") {
                              options2["displayMonth"] = true;
                              }
			      console.log("Continuing after options 3");
                              if (options["date"] == "1") {
                              options2["displayDate"] = true;
                              }
                              if (options["hour"] == "1") {
                              options2["displayHour"] = true;
                              }
                              if (options["minute"] == "1") {
                              options2["displayMinute"] = true;
                              }
                              if (options["bluetooth"] == "1") {
                              options2["displayBluetooth"] = true;
                              }
                              if (options["updates"] == "1") {
                              options2["updates"] = true;
                              }
                              
			      console.log("Copying old options: " + JSON.stringify(options));

			      console.log("Writing new options: " + JSON.stringify(options2));
                              localStorage.setItem('clay-settings', JSON.stringify(options2));
                            */
                        }
                       );


function appMessageAck(e) 
{
    console.log("options sent to Pebble successfully");
}


function appMessageNack(e) 
{
    //    if (e.error) 
    console.log("options not sent to Pebble: ");
}


// The timeline public URL root
var API_URL_ROOT = 'https://timeline-api.getpebble.com/';

function datestring(rem) 
{
    var mon = rem.getMonth()+1;
    var day = rem.getDate();
    var year = rem.getFullYear();
    var hour = rem.getHours();
    var med = "AM";
    var hourint = parseInt(hour);
    if (hourint > 12) {
        hourint -= 12;
        med = 'PM';
    } else if (hourint === 12) {
        med = 'PM';
    } else if (hourint === 0) {
        hourint = 12;
    }
    hour = hourint.toString();
    
    var min = rem.getMinutes();
    var ts = mon + "/" + day + "/" + year + "%20" + hour + ":" + min + ":00%20" + med + "%20-0000";

    return (ts);
}


function timelineRequest(pin, type, callback, topic) {
    // User or shared?
    //  var url = API_URL_ROOT + 'v1/user/pins/' + pin.id;
    var url = API_URL_ROOT + 'v1/shared/pins/' + pin.id;

    // Create XHR
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        console.log('timeline: response received: ' + this.responseText);
        callback(this.responseText);
    };
    xhr.open(type, url);

    // Get token
    //  Pebble.getTimelineToken(function(token) {
    // Add headers
    xhr.setRequestHeader('Content-Type', 'application/json');

    xhr.setRequestHeader('X-API-Key', 'SBggmjrgx33sejzi7q7jbhb52ll7jdee');
    xhr.setRequestHeader('X-Pin-Topics', topic);
    console.log('Using API key');
    
    //    xhr.setRequestHeader('X-User-Token', '' + token);
    console.log('Using User token key');

    // Send
    xhr.send(JSON.stringify(pin));
    console.log('timeline: request sent.');
    //  }, function(error) { console.log('timeline: error getting timeline token: ' + error); });
}


function insertUserPin(pin, callback, topic) {
    //    timelineRequest(pin, 'PUT', callback, topic);
}


function addCalendar(now, title, endtime, al) 
{
    var tzoffset = now.getTimezoneOffset() * 1000 * 60;
    
    var nowgmt = new Date(now.getTime() + tzoffset);
    var ts = datestring(nowgmt);

    var rem2 = new Date(now.getTime() + (1000 * 60 * 60 * endtime));
    var rem2gmt = new Date(rem2.getTime() + tzoffset);
    
    var te = datestring(rem2gmt);
    
    var req = new XMLHttpRequest();
    var url = 'http://mischievous.us/cgi-bin/pebble_calendar.pl?cal=mj&title=' + title + '&start=' + ts + "&end=" + te + "&alarm=" + al.toString();
    
    console.log('Opening URL: ' + url);

    req.open('GET', url, false);
    req.onload = function(e) {
        if (req.readyState == 4 && req.status == 200) {
            if(req.status == 200) {
                console.log('no error');
            }
            else {
                console.log('Error');
            }
        }
    }
    req.send();
}



function pushPinPodChange(t)
{
    var remval = t - (12 * 60 * 60);            // subtract 12 hours
    remval *= 1000;				// make into milliseconds
    var rem = new Date(remval);			// make a date
    rem.setDate(rem.getDate()+3);               // add 3 days

    console.log("podchange t = " + t);
    t *= 1000;
    var now = new Date(t);
    now.setDate(now.getDate()+3);
    console.log("t = " + now);

    // Create the pin
    var pin = {
        "id": "pin-0001",
        "time": now.toISOString(),
        "duration": "480",
        "layout": {
            "type": "calendarPin",
            "title": "Pod Change",
            "tinyIcon": "system://images/SCHEDULED_EVENT"
        },
        "reminders": [
	    {
                "time": rem.toISOString(),
                "layout": {
                    "type": "genericReminder",
                    "title": "Pod Change in 12hr",
                    "tinyIcon": "system://images/SCHEDULED_EVENT"
                }
            }
        ]
    };

    console.log('Inserting change pin in the future: ' + JSON.stringify(pin));
    //  insertUserPinpin, function(responseText) { 
    //          console.log('Result: ' + responseText);
    //      }, 'changes');

    addCalendar(now, 'Pod%20change', 8, -12 * 60);

}


function pushPinPodDead(t)
{
    t *= 1000;                          // milliseconds
    var now = new Date(t);              // date for change
    now.setDate(now.getDate()+3);       // 3 days
    now.setHours(now.getHours()+8);     // plus 8 more hours
    console.log("t = " + now);

    // Remind 15 minutes before 
    var rem = new Date(now.getTime() - (1000 * 60 * 15));
    
    // Create the pin
    var pin = {
        "id": "pin-0002",
        "time": now.toISOString(),
        "layout": {
            "type": "genericPin",
            "title": "Pod Dead",
            "tinyIcon": "system://images/SCHEDULED_EVENT"
        },
        "reminders": [
	    {
                "time": rem.toISOString(),
                "layout": {
                    "type": "genericReminder",
                    "title": "Pod Dead in 15 min",
                    "tinyIcon": "system://images/SCHEDULED_EVENT"
                }
            }
        ]
    };

    console.log('Inserting dead pin in the future: ' + JSON.stringify(pin));
    insertUserPin(pin, function(responseText) { 
        console.log('Result: ' + responseText);
    }, 'changes');

    addCalendar(now, 'Pod%20dead', 0, -15);

}


function pushPinPodUpdate(t)
{
    t *= 1000;
    var comettime = new Date(t);
    var then = new Date();              /* time for pin */
    then.setHours(then.getHours()+1);   /* in future */
    console.log("t = " + then);

    // Create the pin
    var pin = {
        "id": "pin-0003",
        "time": then.toISOString(),
        "layout": {
            "type": "calendarPin", 
            "title": "Pod Change update",
            "tinyIcon": "system://images/SCHEDULED_EVENT"
        },
        "updateNotification": {
            "time": then.toISOString(),
            "layout": {
                "type": "calendarPin", 
                "title": "Dismiss this, find the update in timeline and Update Comet",
                "tinyIcon": "system://images/SCHEDULED_EVENT"
            },
        },
        "createNotification": {
            "layout": {
                "type": "calendarPin", 
                "title": "Dismiss this, find the update in timeline and Update Comet",
                "tinyIcon": "system://images/SCHEDULED_EVENT"
            },
        },
        "actions": [
            {
                "title": "Update comet",
                "type": "openWatchApp",
                "launchCode": (comettime.valueOf()/1000),
            }
        ]
    };

    console.log('Inserting update pin in the future: ' + JSON.stringify(pin));
    insertUserPin(pin, function(responseText) { 
        console.log('Result: ' + responseText);
    }, 'updates');
}




Pebble.addEventListener('appmessage', function(e)
                        {
                            console.log("New message *****");
                            console.log("Received message: " + e.payload);
                            console.log("message = " + e.type + " , " + JSON.stringify(e.payload));
                            for (i in e.payload) {
                                if (i == "arrow") {
                                    console.log("handling arrow request");
                                    var url = e.payload.geturl;
                                    var req = new XMLHttpRequest();
                                    
                                    req.onload = function(e) {
                                        console.log("onload here");
                                        console.log("onload - readystate is " + req.readyState + ", req.status is " + req.status);
                                        if (req.readyState == 4 && req.status == 200) {
                                            if(req.status == 200) {
                                                p = JSON.parse(req.responseText);
                                                console.log("Got: " + p[0].sgv + " " + p[0].direction);
                                                
                                                diff = p[0].sgv - p[1].sgv;
                                                diff = (diff * 90) / 20;
                                                diff = 180 - (diff + 90);
                                                Pebble.sendAppMessage({'arrow': diff});
                                                /*
                                                  switch ( p[0].direction ) {
                                                  case "Flat":
                                                  Pebble.sendAppMessage({'arrow': 90});
                                                  break;
                                                  case "FortyFiveDown":
                                                  Pebble.sendAppMessage({'arrow': 135});
                                                  break;
                                                  case "FortyFiveUp":
                                                  Pebble.sendAppMessage({'arrow': 45});
                                                  break;
                                                  case "SingleUp":
                                                  Pebble.sendAppMessage({'arrow': 0});
                                                  break;
                                                  case "SingleDown":
                                                  Pebble.sendAppMessage({'arrow': 180});
                                                  break;
                                                  case "DoubleUp":
                                                  Pebble.sendAppMessage({'arrow': -1});
                                                  break;
                                                  case "DoubleDown":
                                                  Pebble.sendAppMessage({'arrow': 181});
                                                  break;
                                                  }
                                                */
			                        now = new Date();
			                        nowTime = now.getTime() / 1000;
                                                nowTime = Math.trunc(nowTime);
			                        console.log("Now in seconds:" + nowTime);

                                                // Calculate next update time 

                                                var p0 = new Date(p[0].date).getTime();
                                                console.log("p0 is " + p0);
                                                var p1 = new Date(p[1].date).getTime();
                                                console.log("p0 is " + p0);
                                                var lastp = (p0 - p1) / 1000;
                                                lastp = Math.trunc(lastp);
			                        console.log("last period in seconds: " + lastp);
                                                var next = (p0 / 1000) + lastp;
                                                next = Math.trunc(next);
			                        console.log("next in seconds: " + next);
                                                while (next < nowTime) {
                                                    next += lastp;
			                            console.log("next is now seconds: " + Math.trunc(next));
                                                }

			                        if (p0 < nowTime) {
			                            Pebble.sendAppMessage({'sgv': 0});
			                        } else {
                                                    Pebble.sendAppMessage({'sgv': p[0].sgv});
                                                    Pebble.sendAppMessage({'next': Math.trunc(next - nowTime)});
			                        }
                                            }
                                            else {
                                                Pebble.sendAppMessage({'sgv': 0});
                                                console.log('Error');
                                            }
                                        }
                                    }
                                    req.open('GET',
                                             "https://peacock.place/api/v1/entries/sgv.json?token=raj-6680c763a5eb003d&count=2",
                                             false);
                                    
                                    console.log("req.send returns: " + req.send());
                                    return;
                                }
                                
	                        if (i == "ack" || i == "podchange") {
                                    console.log("index = " + e.payload.ack);
                                    
                                    var podchange = e.payload.ack;
                                    
	                            console.log("Received message: " + podchange);
                                    
                                    //	    var options = JSON.parse(localStorage.getItem('clay-settings'));
                                    //            if (options.updates == '1') {
                                    //                                Pebble.timelineSubscribe('updates',
                                    //                                                         function () 
                                    //                                                         {
                                    //                                                             console.log('Subscribed to updates'//);
                                    //                                                         },
                                    //                                                         function (errorString) 
                                    //                                                         {
                                    //                                                             console.log('Error subscribing to u//pdates: ' + errorString);
                                    //                                                         }
                                    //                                  );
                                    //            }
                                    //            if (options.changeit == '1') {
                                    //                                pushPinPodChange(podchange);
                                    //                                pushPinPodDead(podchange);
                                    //                                pushPinPodUpdate(podchange);
                                    //            }
                                    
                                    //            options["podchange"] = podchange;
                                    //            options["changeit"] = false; /* init */
                                    //            options["ago"] = "0"; /* init */
                                    //            localStorage.setItem('clay-settings', JSON.stringify(options));
                                    
                                    //            Pebble.sendAppMessage({'ack': true});
	                        }
                            }
                        }
                       );

console.log("JS code end");
