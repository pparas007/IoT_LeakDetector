var TB_ADDRESS = "10.6.32.117" //Jinchie's
var TB_PORT = 8080
var TB_TOKEN =
"eyJhbGciOiJIUzUxMiJ9.eyJzdWIiOiJqeGllQHRjZC5pZSIsInNjb3BlcyI6WyJURU5BTlRfQURNSU4iXSwidXNlcklkIjoiYTdkZTRiNjAtZGQ0Ny0xMWU4LTk2ZTQtYjE5ZGRmNzRmY2RmIiwiZmlyc3ROYW1lIjoiamluY2hpIiwibGFzdE5hbWUiOiJ4aWUiLCJlbmFibGVkIjp0cnVlLCJpc1B1YmxpYyI6ZmFsc2UsInRlbmFudElkIjoiNzExOTRjNjAtZGQ0Ny0xMWU4LTk2ZTQtYjE5ZGRmNzRmY2RmIiwiY3VzdG9tZXJJZCI6IjEzODE0MDAwLTFkZDItMTFiMi04MDgwLTgwODA4MDgwODA4MCIsImlzcyI6InRoaW5nc2JvYXJkLmlvIiwiaWF0IjoxNTQyMzc5MjQyLCJleHAiOjE1NTEzNzkyNDJ9.UMKLHlqJqeu2paypcWrVin-Fgi8KPlBbmlqA_IUqmLOsSijTKGT8ApSbwO0_C57HcbIBEAzLdfTYpleY8SnqfA" //--Jinchi previous token

var DEVICE_IDS = [
    "e156dd40-ed90-11e8-98bd-7fab095dcff9"
];

var MAX_RECORDS=10
var LOW_WATER_LEVEL="LOW"
var MEDIUM_WATER_LEVEL="MEDIUM"
var HIGH_WATER_LEVEL="HIGH"

var NO_FLOW="NO_FLOW"
var FLUSHING_SLOW="SLOW"
var FLUSHING_FAST="FAST"

var LAST_WATER_FLOW_STATE="NONE"
var LAST_WATER_LEVEL_STATE="NONE"

var SUSPECT_STATE=false
var SUSPECT_STATE_TIME_INTERVAL=0
var DEVICE = 0;
var SLEEP_TIME=2000

var TO_MAIL='panezveg@tcd.ie'
var SUBJECT='Leak detetcted!'
var TEXT='Hey Gustavo, Leak detected in your tank. Closing the valve!'

var nodemailer = require('nodemailer');

const cluster = require('cluster');

var i=0
if (cluster.isMaster) {
    if(i==0)
    {   
        console.log('forking')
        cluster.fork();
        i++;
    }
    
}
else{
    console.log('Setting up child thread to detect leak')        
    setInterval(detectLeakLogic,SLEEP_TIME)
}
function detectLeakLogic() {
    console.log(`\n\nChecking leak . . . `);        
    if(LAST_WATER_FLOW_STATE==FLUSHING_FAST && LAST_WATER_LEVEL_STATE==LOW_WATER_LEVEL){
        console.log('\nsuspect stage . . .')
        SUSPECT_STATE=true;
    }else{
        if(SUSPECT_STATE==true) {
            openValve()
        }
        SUSPECT_STATE_TIME_INTERVAL=0
        SUSPECT_STATE=false;        
    }

    if(SUSPECT_STATE==true){
        SUSPECT_STATE_TIME_INTERVAL++
        if(SUSPECT_STATE_TIME_INTERVAL==3){
            console.log('\nLeak confirmed\nClosing the valve & Sending the mail!')
            closeValve()
            sendMail(TO_MAIL, SUBJECT, TEXT)
        }
    }
    
}

function sendMail(toEmail, subject, text) {
    var transporter = nodemailer.createTransport({
        service: 'gmail',
        auth: {
            user: 'jinchi1995@gmail.com',
            pass: '2014141453190'
        }
    });

    var mailOptions = {
        from: 'jinchi1995@gmail.com',
        to: toEmail,
        subject: subject,
        text: text
    };

    transporter.sendMail(mailOptions, function (error, info) {
        if (error) {
            console.log(error);
        } else {
            console.log('Email sent: ' + info.response);
        }
    });
}

function closeValve() {
    var deviceId = DEVICE_IDS[DEVICE]
    var request = require("request");
    var url = "http://" + TB_ADDRESS+":" + TB_PORT + "/api/plugins/rpc/oneway/" + deviceId;

    // The JSON RPC description must match that expected in tb_pubsub.c
    var req = {
        "method" : "closeValve",
        "params" : {
            "ledno" : "dummy"
        }

    };
    console.log("RPC Request: " + url + ": " + JSON.stringify(req));

    request({
        url: url,
        method: "POST",
        json: req,
        headers: {
            "X-Authorization": "Bearer " + TB_TOKEN,
        }
    }, function (error, response, body) {
        if (!error && response.statusCode === 200) {
            console.log("OK" + ((typeof body != 'undefined') ? ": " + body : ""));
        }
        else {
            console.log("error: " + error)
            console.log("response.statusCode: " + response.statusCode)
            console.log("response.statusText: " + response.statusText)
        }
    });
}

function openValve() {
    var deviceId = DEVICE_IDS[DEVICE]
    var request = require("request");
    var url = "http://" + TB_ADDRESS+":" + TB_PORT + "/api/plugins/rpc/oneway/" + deviceId;

    var req = {
        "method" : "openValve",
        "params" : {
            "ledno" : "dummy"
        }
    };

    console.log("RPC Request: " + url + ": " + JSON.stringify(req));

    request({
        url: url,
        method: "POST",
        json: req,
        headers: {
            "X-Authorization": "Bearer " + TB_TOKEN,
        }
    }, function (error, response, body) {
        if (!error && response.statusCode === 200) {
            console.log("OK" + ((typeof body != 'undefined') ? ": " + body : ""));
        }
        else {
            console.log("error: " + error)
            console.log("response.statusCode: " + response.statusCode)
            console.log("response.statusText: " + response.statusText)
        }
    });
}

function recordWaterFlow(waterFlow){
    LAST_WATER_FLOW_STATE=waterFlow
}

function recordWaterLevel(waterLevel){
    LAST_WATER_LEVEL_STATE=waterLevel
}


function processTelemetryData(deviceId, data) {
    console.log("Telemetry from " + deviceId + " : " + JSON.stringify(data));

    if (deviceId == DEVICE_IDS[DEVICE]) {
        console.log('Inside IF: '+data)
        if (typeof data.frc !== 'undefined') {
            waterLevel=data.frc[0][1]
            console.log('\n\nWater level : '+waterLevel)
            recordWaterLevel(waterLevel);
        }
        else if (typeof data.wtr !== 'undefined') {
            waterFlow=data.wtr[0][1]
            console.log('\n\nWater Flow : '+waterFlow)
            recordWaterFlow(waterFlow);
        }
    }
}

var WebSocketClient = require('websocket').client;
var client = new WebSocketClient();

client.on('connectFailed', function(error) {
    console.log('Connect Error: ' + error.toString());
});

client.on('connect', function(connection) {

    console.log('WebSocket Client Connected');

    connection.on('error', function(error) {
        console.log("Connection Error: " + error.toString());
    });

    connection.on('close', function() {
        console.log('echo-protocol Connection Closed');
    });

    connection.on('message', function(message) {
        if (message.type === 'utf8') {
            var rxObj = JSON.parse(message.utf8Data);
            if (typeof rxObj.subscriptionId !== 'undefined') {
                processTelemetryData(DEVICE_IDS[rxObj.subscriptionId], rxObj.data);
            }
        }
    });

    function subscribe(deviceIdx) {
        var req = {
            tsSubCmds: [
                {
                    entityType: "DEVICE",
                    entityId: DEVICE_IDS[deviceIdx],
                    scope: "LATEST_TELEMETRY",
                    cmdId: deviceIdx
                }
            ],
            historyCmds: [],
            attrSubCmds: []
        };
        console.log("Subscribing to " + DEVICE_IDS[deviceIdx]);
        connection.sendUTF(JSON.stringify(req));
    }

    if (connection.connected) {
        subscribe(DEVICE);
    }
});

client.connect("ws://" + TB_ADDRESS + ":" + TB_PORT + "/api/ws/plugins/telemetry?token=" + TB_TOKEN);
