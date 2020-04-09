var dataTable = new DataTable("#table", {
    sortable: false,
    labels: {
        perPage: "Servers per page {select}",
        noRows: "",
        info: ""
    }
});
var textServer = null;
var timeToNextCall = 1000;
var maxValue = 100;
var canvas = document.getElementById("speedTest");
var ctx = canvas.getContext("2d");
var minAngle = Math.PI * 1.5;
var r = canvas.width / 2 - 20;
var selectedServer;
var avgDownload = 0;
var avgUpload = 0;
var xmlDoc;
var canStart;
var isReadResults;
var isDownloadSet;
var isUploadSet;
var isError;

//Draw variables.
var target = 0;
var currentValue = 0;

//Draws the speedometer.
draw();

//If the screen resizes then redraw.
window.onresize = function() {
    draw();
}

function startSpeedTest(id) {
    timeToNextCall = 1000;
    isReadResults = true;
    isDownloadSet = false;
    isUploadSet = false;
    isError = false;
    //Disable the buttons.
    var startButton = document.getElementById(id);
    startButton.disabled = true;
    startButton.setAttribute('style', 'opacity: 0.5;');
    var changeServerButton = document.getElementById("changeServer");
    changeServerButton.disabled = true;
    changeServerButton.setAttribute('style', 'opacity: 0.5;');
    //If there is a selected server sends it to the controller.
    var link = window.location.href + '/start';
    if (selectedServer != null) {
        url = selectedServer.getAttribute('url');
        parameters = "?url=" + url;
        link = link + parameters;
    }
    if (xmlDoc == null) {
        writeToTable();
    }
    //Set the elements to indicate for the speed test start.
    document.getElementById("downloadAvg").innerHTML = "Download: -";
    document.getElementById("uploadAvg").innerHTML = "Upload: - ";
    sendStart(link);
}

function sendStart(link) {
    if (canStart) {
        var xhr = new XMLHttpRequest();

        xhr.open("GET", link);
        xhr.send();

        xhr.onreadystatechange = function() {
            if (this.readyState === 4 && this.status === 200) {}
        };
        setTimeout(readSpeedResults, 1000);
    } else {
        setTimeout(sendStart, 1000, link);
    }
}

//Calls for the json about the speed test.
function readSpeedResults() {
    callJson(function(response) {
        var json = JSON.parse(response);
        if (json.state == "CHECKING_CONNECTION") {
            //Draw the gauge at 0.
            if (target != 0) {
                target = 0;
                draw();
            }
        } else if (json.state == "FINDING_SERVER") {
            //Draw the gauge at 0.
            if (target != 0) {
                target = 0;
                draw();
            }
        } else if (json.state == "TESTING_DOWNLOAD") {
            //Get the text about the server from the start of download.
            if (textServer == null) {
                textServer = "finding..";
                setTextAboutServer(json.serverURL);
            }
            //If the download information is not set then set it.
            if (isReadResults) {
                isReadResults = false;
                for (var i = 0; i < 3; i++) {
                    readSpeedResults();
                }
                timeToNextCall = 0;
            }
            //Redraw if the average download changes.
            if (avgDownload != json.avgDownloadSpeed) {
                avgDownload = json.avgDownloadSpeed;
                target = convertToMbps(avgDownload);
                draw();
            }
        } else if (json.state == "TESTING_UPLOAD") {
            //Set the information about the upload testing and the results of the download.
            if (!isDownloadSet) {
                isDownloadSet = true;
                var download = document.getElementById('downloadAvg');
                download.innerHTML = "Download : " + Math.round(convertToMbps(avgDownload) * 100) / 100 + "Mbps";
            }
            //Redraw if the average upload changes.
            if (avgUpload != json.avgUploadSpeed) {
                avgUpload = json.avgUploadSpeed;
                target = convertToMbps(avgUpload);
                draw();
            }
        } else if (json.state == "FINISHED") {
            //If the target is not 0 set it to zero and write about the upload results.
            if (target != 0) {
                document.getElementById("startSpeedTest").disabled = false;
                document.getElementById("changeServer").disabled = false;
                document.getElementById("startSpeedTest").setAttribute('style', " ;");
                document.getElementById("changeServer").setAttribute('style', " ;");
                var upload = document.getElementById('uploadAvg');
                upload.innerHTML = "Upload : " + Math.round(convertToMbps(avgUpload) * 100) / 100 + "Mbps";
                target = 0;
                draw();
            }
            if (document.getElementById("changeServer").disabled || document.getElementById("startSpeedTest").disabled) {
                document.getElementById("startSpeedTest").disabled = false;
                document.getElementById("changeServer").disabled = false;
                document.getElementById("startSpeedTest").setAttribute('style', " ;");
                document.getElementById("changeServer").setAttribute('style', " ;");
            }
            return;
        } else if (json.state == "COOLDOWN") {
            if (!isDownloadSet) {
                isDownloadSet = true;
                var download = document.getElementById('downloadAvg');
                download.innerHTML = "Download : " + Math.round(convertToMbps(avgDownload) * 100) / 100 + "Mbps";
            }
            //Draw the gauge at 0.
            if (target != 0) {
                target = 0;
                draw();
            }
        } else if (json.state == "ERROR") {
            if (!isError) {
                isError = true;
                setError(json.error);
                document.getElementById("startSpeedTest").disabled = false;
                document.getElementById("changeServer").disabled = false;
                document.getElementById("startSpeedTest").setAttribute('style', " ;");
                document.getElementById("changeServer").setAttribute('style', " ;");
            }
            //Draw the gauge at 0.
            if (target != 0) {
                target = 0;
                draw();
            }
            return;
        }

        setTimeout(readSpeedResults, timeToNextCall);
    });
}

//Set the text about server for an url.
function setTextAboutServer(url) {
    var servers = xmlDoc.getElementsByTagName('server');
    for (var i = 0; i < xmlDoc.getElementsByTagName('server').length; i++) {
        if (servers[i].getAttribute('url').includes(url)) {
            var city = servers[i].getAttribute('name');
            var sponsor = servers[i].getAttribute('sponsor');
            var country = servers[i].getAttribute('country');
            var host = servers[i].getAttribute('host');
            setIpAdress(host);
            textServer = sponsor + " from " + city + ", " + country;
            selectedServer = servers[i];
            break;
        }
    }
}

//Calls for the speedtest.json file form backend.
function callJson(callback) {
    var directory = window.location.href + '/getJSON';
    var xobj = new XMLHttpRequest();
    xobj.open('GET', directory, true); // Replace 'my_data' with the path to your file
    xobj.onreadystatechange = function() {
        if (xobj.readyState == 4 && xobj.status == "200") {
            // Required use of an anonymous callback as .open will NOT return a value but simply returns undefined in asynchronous mode
            callback(xobj.responseText);
        }
    };
    xobj.send(null);
}

//Converts the speed to Mbps.
function convertToMbps(value) {
    return value / 125000;
}

/*Draw functions*/

//Main draw function. 
function draw() {
    var val = currentValue;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    var angle = Math.PI;
    if (val < maxValue) {
        angle = Math.PI * val / maxValue;
    }
    canvas.style.width = '100%';
    canvas.style.height = '100%';
    canvas.width = canvas.offsetWidth;
    canvas.height = canvas.offsetWidth * 0.5;
    r = canvas.width / 2 - canvas.width / 2 * 0.05;
    drawGauge(angle);
    drawPointer(angle);
    drawGaugeSpeeds();
    drawSpeed(target);
    if (Math.round(target * 10) != Math.round(currentValue * 10)) {
        currentValue += inclineCalculation(target, val);
        window.requestAnimationFrame(draw);
    }

}

function inclineCalculation(target, val) {
    var dif = target - val;
    if (Math.abs(dif) > 0.1) {
        return dif / 50;
    } else if (dif > 0) {
        return 0.01;
    } else if (dif < 0) {
        return -0.01;
    }
}

function drawGauge(angle) {
    angle = Math.PI + angle;
    ctx.lineWidth = canvas.width * 0.025;
    ctx.strokeStyle = "#0054A6";

    ctx.beginPath();
    ctx.arc(canvas.width / 2, canvas.height, r, angle, 0);
    ctx.globalAlpha = 0.2;
    ctx.stroke();
    ctx.beginPath();
    ctx.arc(canvas.width / 2, canvas.height, r, Math.PI, angle);
    ctx.globalAlpha = 1;
    ctx.stroke();
}

function drawPointer(angle) {
    var offset = canvas.width / 20;
    angle = minAngle - angle;
    ctx.beginPath();
    ctx.globalAlpha = 1;
    var coord = getCoordinates(angle, r);
    var coord1 = getCoordinates(angle + 0.5 * Math.PI, canvas.width * 0.03);
    var coord2 = getCoordinates(angle - 0.5 * Math.PI, canvas.width * 0.03);
    ctx.moveTo(coord1[0], coord1[1]);
    ctx.lineTo(coord[0], coord[1]);
    ctx.lineTo(coord2[0], coord2[1]);
    var grd = ctx.createLinearGradient(coord[0], coord[1], canvas.width / 2, canvas.height);
    grd.addColorStop(0, "#0054A6");
    grd.addColorStop(1, "white");
    ctx.fillStyle = grd;
    ctx.fill()
}

function drawGaugeSpeeds() {
    ctx.fillStyle = "#000000";
    var fontSize = 0.04;
    var size = canvas.width * fontSize;
    for (i = 0; i <= 10; i++) {
        ctx.font = size + "px Arial";
        var angle = Math.PI * 1.5 - Math.PI * i / 10;
        var offset = canvas.width * 0.06;
        var coord = getCoordinates(angle, r - offset);
        ctx.textAlign = "center";
        ctx.fillText(i * 10, coord[0], coord[1]);
    }
}

function drawSpeed(value) {
    ctx.fillStyle = "#000000";
    var fontSize = 0.04;
    var size = canvas.width * fontSize;
    ctx.font = size + "px Arial";
    ctx.textAlign = "center";
    ctx.fillText(Math.round(value * 100) / 100, canvas.width / 2, canvas.height);
}

function getCoordinates(angle, rad) {
    return [canvas.width / 2 + rad * Math.sin(angle), canvas.height + rad * Math.cos(angle)]
}

/*Modal functions */
var newRows = [];

//Add server elements to an array.
function addItem(country, city, name) {
    var spanCountry = document.createElement("span");
    var spanCity = document.createElement("span");
    var spanName = document.createElement("span");
    spanCountry.setAttribute('class', 'selectCountry');
    spanCity.setAttribute('class', 'selectCity');
    spanName.setAttribute('class', 'selectName');
    spanCountry.appendChild(document.createTextNode(country));
    spanCity.appendChild(document.createTextNode(city));
    spanName.appendChild(document.createTextNode(name));
    var info = [spanCountry.outerHTML, spanCity.outerHTML, spanName.outerHTML];
    newRows.push(info);
}

//Select function from table.
function select(id) {
    var text = document.getElementById("textChooseServer");
    var servers = xmlDoc.getElementsByTagName('server');
    for (var i = 0; i < servers.length; i++) {
        if (servers[i].getAttribute('id') === id) {
            var city = servers[i].getAttribute('name');
            var sponsor = servers[i].getAttribute('sponsor');
            var country = servers[i].getAttribute('country');
            var host = servers[i].getAttribute('host');
            setIpAdress(host);
            selectedServer = servers[i];
            textServer = sponsor + " from " + city + ", " + country;
            text.innerHTML = "Selected server is " + textServer;
            break;
        }
    }
    modal.style.display = "none";
}

function writeToTable() {
    document.getElementById("textChooseServer").innerHTML = "Getting the server list...";
    document.getElementById("textChooseServer").style.color = "";
    showLoading();
    getServerList(function(response) {
        let parser = new DOMParser();
        xmlDoc = parser.parseFromString(response, "application/xml");
        var servers = xmlDoc.getElementsByTagName('server');
        //Put the data to the table.
        for (var i = 0; i < xmlDoc.getElementsByTagName('server').length; i++) {
            var city = servers[i].getAttribute('name');
            var country = servers[i].getAttribute('country');
            var sponsor = servers[i].getAttribute('sponsor');
            addItem(country, city, sponsor);
        }
        dataTable.rows().add(newRows);
        //Set attribute for the table.
        for (var i = 0; i < xmlDoc.getElementsByTagName('server').length; i++) {
            var id = servers[i].getAttribute('id');
            dataTable.activeRows[i].setAttribute('id', id);
            dataTable.activeRows[i].setAttribute('onclick', 'select(this.id);');
            dataTable.activeRows[i].setAttribute('class', 'trRow');
        }
        hideLoading();
        document.getElementById("textChooseServer").innerHTML = "Choose a server";
    });

}

function showLoading() {
    var e = document.getElementById("loading");
    e.setAttribute('style', 'display: flex;flex-direction: column-reverse;align-items: center; margin-top: 100px; margin-bottom: 100px;');
    var list = document.getElementById("tableContent");
    list.setAttribute('style', 'display: none');
}
//Hide the loading element.
function hideLoading() {
    var e = document.getElementById("loading");
    e.setAttribute('style', 'display: none;');
    var list = document.getElementById("tableContent");
    list.setAttribute('style', 'display: ');
}

//Callback for the server list.
function getServerList(callback) {
    canStart = false;
    var xhr = new XMLHttpRequest();
    xhr.open("GET", window.location.href + '/getServers', true);
    xhr.withCredentials = true;
    xhr.send();
    xhr.onreadystatechange = function() {
        if (this.readyState === 4 && this.status === 200) {
            if (xhr.response == "Error") {
                var chooseText = document.getElementById("textChooseServer");
                errorMessage("There was a problem getting the server list.");
                var e = document.getElementById("loading");
                e.setAttribute('style', 'display: none;');
            } else {
                callback(xhr.responseText);
            }
        }
        canStart = true;
    };
}

function setIpAdress(url) {
    getIpAddress(url, function(callback) {
        sponsor = selectedServer.getAttribute('sponsor');
        document.getElementById("sponsor").innerHTML = sponsor;
        document.getElementById("ip").innerHTML = callback;
    });
}

function getIpAddress(url, callback) {
    var xhr = new XMLHttpRequest();
    var link = window.location.href + '/getIp';
    parameters = "?url=" + url;
    link = link + parameters;

    xhr.open("GET", link, true);
    xhr.withCredentials = true;
    xhr.send();
    xhr.onreadystatechange = function() {
        if (this.readyState === 4 && this.status === 200) {
            if (xhr.response == "Error") {
                setError("Internet connection is required to use this application.");
            } else {
                callback(xhr.responseText);
            }
        }
        canStart = true;
    };
}

// Get the modal
var modal = document.getElementById("find-servers");

// Get the button that opens the modal
var btn = document.getElementById("changeServer");
// Get the <span> element that closes the modal
var span = document.getElementsByClassName("closeModal")[0];

// When the user clicks the button, open the modal 
btn.onclick = function() {
    if (xmlDoc == null) {
        writeToTable();
    }
    modal.style.display = "block";
}

// When the user clicks on <span> (x), close the modal
span.onclick = function() {
        modal.style.display = "none";
    }
    // When the user clicks anywhere outside of the modal, close it
window.onclick = function(event) {
    if (event.target == modal) {
        modal.style.display = "none";
    }
}

function setError(errorMessage) {
    var errorList = document.getElementsByClassName("notification error");
    //Checks if the same error is not already put
    if (errorList.length > 0) {
        for (var i = 0; i < errorList.length; i++) {
            if (errorList[i].getElementsByClassName("notification-text")[0].innerHTML == errorMessage) {
                return;
            }
        }
    }

    var error = document.createElement("div");
    var message = document.createElement("div");
    var close = document.createElement("div");
    error.setAttribute('class', 'notification error');
    error.setAttribute('onclick', 'this.remove()');
    message.setAttribute('class', 'notification-text');
    message.innerHTML = errorMessage;
    close.setAttribute('class', 'close');
    error.appendChild(message);
    error.appendChild(close);
    var list = document.getElementsByClassName("notification-list");
    list[0].appendChild(error);
}