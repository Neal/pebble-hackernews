var maxAppMessageBuffer = 100;
var maxAppMessageTries = 3;
var appMessageRetryTimeout = 3000;
var appMessageTimeout = 100;
var httpTimeout = 12000;
var appMessageQueue = [];
var stories = {};

var ENDPOINTS = {
	'FRONTPAGE': 0,
	'NEWPOSTS': 1,
	'BESTPOSTS': 2
};

var API_URLS = {
	[ENDPOINTS.FRONTPAGE]: 'http://hnify.herokuapp.com/get/top',
	[ENDPOINTS.NEWPOSTS]: 'http://hnify.herokuapp.com/get/newest',
	[ENDPOINTS.BESTPOSTS]: 'http://hnify.herokuapp.com/get/best',
	'clipped': 'http://clipped.me/algorithm/clippedapi.php?url='
};

function sendAppMessage() {
	if (appMessageQueue.length > 0) {
		currentAppMessage = appMessageQueue[0];
		currentAppMessage.numTries = currentAppMessage.numTries || 0;
		currentAppMessage.transactionId = currentAppMessage.transactionId || -1;
		if (currentAppMessage.numTries < maxAppMessageTries) {
			console.log('Sending AppMessage to Pebble: ' + JSON.stringify(currentAppMessage.message));
			Pebble.sendAppMessage(
				currentAppMessage.message,
				function(e) {
					appMessageQueue.shift();
					setTimeout(function() {
						sendAppMessage();
					}, appMessageTimeout);
				}, function(e) {
					console.log('Failed sending AppMessage for transactionId:' + e.data.transactionId + '. Error: ' + e.data.error.message);
					appMessageQueue[0].transactionId = e.data.transactionId;
					appMessageQueue[0].numTries++;
					setTimeout(function() {
						sendAppMessage();
					}, appMessageRetryTimeout);
				}
			);
		} else {
			console.log('Failed sending AppMessage for transactionId:' + currentAppMessage.transactionId + '. Bailing. ' + JSON.stringify(currentAppMessage.message));
		}
	}
}

function hackernews(endpoint) {
	var xhr = new XMLHttpRequest();
	xhr.open('GET', API_URLS[endpoint], true);
	xhr.timeout = httpTimeout;
	xhr.onload = function(e) {
		if (xhr.readyState == 4) {
			if (xhr.status == 200) {
				if (xhr.responseText) {
					res = JSON.parse(xhr.responseText);
					stories = res.stories;
					stories.forEach(function (element, index, array) {
						title = element.title.substring(0,23);
						subtitle = element.points + ' points • ' + element.num_comments + ' comments';
						appMessageQueue.push({'message': {'endpoint': endpoint, 'index': index, 'title': title, 'subtitle': subtitle}});
					});
				} else {
					console.log('Invalid response received! ' + JSON.stringify(xhr));
					appMessageQueue.push({'message': {'endpoint': endpoint, 'title': 'Invalid response!'}});
				}
			} else {
				console.log('Request returned error code ' + xhr.status.toString());
				appMessageQueue.push({'message': {'endpoint': endpoint, 'title': 'HTTP/1.1 ' + xhr.statusText}});
			}
		}
		sendAppMessage();
	}
	xhr.ontimeout = function() {
		console.log('HTTP request timed out');
		appMessageQueue.push({'message': {'endpoint': endpoint, 'title': 'Request timed out!'}});
		sendAppMessage();
	};
	xhr.onerror = function() {
		console.log('HTTP request return error');
		appMessageQueue.push({'message': {'endpoint': endpoint, 'title': 'Failed to connect!'}});
		sendAppMessage();
	};
	xhr.send(null);
}

function clipped(url, endpoint) {
	var xhr = new XMLHttpRequest();
	xhr.open('GET', API_URLS.clipped + url, true);
	xhr.timeout = httpTimeout;
	xhr.onload = function(e) {
		if (xhr.readyState == 4) {
			if (xhr.status == 200) {
				if (xhr.responseText) {
					try {
						res = JSON.parse(xhr.responseText);
						title = res.title || '';
						summary = res.summary.join(' ') || '';
						for (var i = 0; i <= Math.floor(summary.length/maxAppMessageBuffer); i++) {
							appMessageQueue.push({'message': {'endpoint': endpoint, 'summary': summary.substring(i * maxAppMessageBuffer, i * maxAppMessageBuffer + maxAppMessageBuffer)}});
						}
						appMessageQueue.push({'message': {'endpoint': endpoint, 'summary': true, 'title': title.substring(0,60)}});
					} catch(e) {
						console.log('Caught error: ' + JSON.stringify(e));
						appMessageQueue.push({'message': {'endpoint': endpoint, 'summary':true}});
						appMessageQueue.push({'message': {'endpoint': endpoint, 'summary':true, 'title': 'Unable to summarize! :('}});
					}
				} else {
					console.log('Invalid response received! ' + JSON.stringify(xhr));
					appMessageQueue.push({'message': {'endpoint': endpoint, 'summary':true, 'title': 'Invalid response!'}});
				}
			} else {
				console.log('Request returned error code ' + xhr.status.toString());
				appMessageQueue.push({'message': {'endpoint': endpoint, 'summary':true, 'title': 'HTTP/' + xhr.statusText}});
			}
		}
		sendAppMessage();
	}
	xhr.ontimeout = function() {
		console.log('HTTP request timed out');
		appMessageQueue.push({'message': {'endpoint': endpoint, 'summary':true, 'title': 'Request timed out!'}});
		sendAppMessage();
	};
	xhr.onerror = function() {
		console.log('HTTP request return error');
		appMessageQueue.push({'message': {'endpoint': endpoint, 'summary':true, 'title': 'Failed to connect!'}});
		sendAppMessage();
	};
	xhr.send(null);
}

Pebble.addEventListener('ready', function(e) {});

Pebble.addEventListener('appmessage', function(e) {
	console.log('AppMessage received from Pebble: ' + JSON.stringify(e.payload));
	if (e.payload.summary) {
		clipped(stories[e.payload.index].link, e.payload.endpoint);
	} else if (typeof(e.payload.endpoint) != 'undefined') {
		hackernews(e.payload.endpoint);
	} else {
		appMessageQueue = [];
	}
});

