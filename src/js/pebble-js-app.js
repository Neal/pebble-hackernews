var options = {
	appMessage: {
		maxBuffer: 480,
		maxTries: 3,
		retryTimeout: 3000,
		timeout: 100
	},
	http: {
		timeout: 20000
	}
};

var appMessageQueue = [];
var stories = {};

var ENDPOINTS = {
	TOP: 0,
	NEW: 1,
	BST: 2,
	ASK: 3
};

var CLIPPED_API_URL = 'http://clipped.me/algorithm/clippedapi.php?url=';
var BASE_HN_API_URL = 'http://hndroidapi.appspot.com/';
var HN_API_URL_OPTS = '/format/json/page/?appid=&callback=&guid=a21d75c1e58e4c86ae6ea6369b2c4172';

var HN_API_URLS = {
	[ENDPOINTS.TOP]: BASE_HN_API_URL + 'news' + HN_API_URL_OPTS,
	[ENDPOINTS.NEW]: BASE_HN_API_URL + 'newest' + HN_API_URL_OPTS,
	[ENDPOINTS.BST]: BASE_HN_API_URL + 'best' + HN_API_URL_OPTS,
	[ENDPOINTS.ASK]: BASE_HN_API_URL + 'ask' + HN_API_URL_OPTS,
};

function sendAppMessageQueue() {
	if (appMessageQueue.length > 0) {
		currentAppMessage = appMessageQueue[0];
		currentAppMessage.numTries = currentAppMessage.numTries || 0;
		currentAppMessage.transactionId = currentAppMessage.transactionId || -1;
		if (currentAppMessage.numTries < options.appMessage.maxTries) {
			console.log('Sending AppMessage to Pebble: ' + JSON.stringify(currentAppMessage.message));
			Pebble.sendAppMessage(
				currentAppMessage.message,
				function(e) {
					appMessageQueue.shift();
					setTimeout(function() {
						sendAppMessageQueue();
					}, options.appMessage.timeout);
				}, function(e) {
					console.log('Failed sending AppMessage for transactionId:' + e.data.transactionId + '. Error: ' + e.data.error.message);
					appMessageQueue[0].transactionId = e.data.transactionId;
					appMessageQueue[0].numTries++;
					setTimeout(function() {
						sendAppMessageQueue();
					}, options.appMessage.retryTimeout);
				}
			);
		} else {
			appMessageQueue.shift();
			console.log('Failed sending AppMessage for transactionId:' + currentAppMessage.transactionId + '. Bailing. ' + JSON.stringify(currentAppMessage.message));
		}
	} else {
		console.log('AppMessage queue is empty.');
	}
}

function hackernews(endpoint) {
	var xhr = new XMLHttpRequest();
	xhr.open('GET', HN_API_URLS[endpoint], true);
	xhr.timeout = options.http.timeout;
	xhr.onload = function(e) {
		if (xhr.readyState == 4) {
			if (xhr.status == 200) {
				if (xhr.responseText) {
					res = JSON.parse(xhr.responseText);
					stories = res.items;
					stories.forEach(function (element, index, array) {
						title = decodeURIComponent(element.title.substring(0,23));
						subtitle = element.score + ' • ' + element.comments;
						if (title != 'NextId') // until we add next page support
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
		sendAppMessageQueue();
	}
	xhr.ontimeout = function() {
		console.log('HTTP request timed out');
		appMessageQueue.push({'message': {'endpoint': endpoint, 'title': 'Request timed out!'}});
		sendAppMessageQueue();
	};
	xhr.onerror = function() {
		console.log('HTTP request return error');
		appMessageQueue.push({'message': {'endpoint': endpoint, 'title': 'Failed to connect!'}});
		sendAppMessageQueue();
	};
	xhr.send(null);
}

function clipped(url, endpoint) {
	var xhr = new XMLHttpRequest();
	xhr.open('GET', CLIPPED_API_URL + url, true);
	xhr.timeout = options.http.timeout;
	xhr.onload = function(e) {
		if (xhr.readyState == 4) {
			if (xhr.status == 200) {
				if (xhr.responseText) {
					try {
						res = JSON.parse(xhr.responseText);
						title = res.title || '';
						summary = res.summary.join(' ') || '';
						for (var i = 0; i <= Math.floor(summary.length/options.appMessage.maxBuffer); i++) {
							appMessageQueue.push({'message': {'endpoint': endpoint, 'summary': summary.substring(i * options.appMessage.maxBuffer, i * options.appMessage.maxBuffer + options.appMessage.maxBuffer)}});
						}
						appMessageQueue.push({'message': {'endpoint': endpoint, 'summary': true, 'title': title.substring(0,60)}});
					} catch(e) {
						console.log('Caught error: ' + JSON.stringify(e));
						console.log(JSON.stringify(xhr));
						appMessageQueue.push({'message': {'endpoint': endpoint, 'summary': ''}});
						appMessageQueue.push({'message': {'endpoint': endpoint, 'summary': true, 'title': 'Unable to summarize! :('}});
					}
				} else {
					console.log('Invalid response received! ' + JSON.stringify(xhr));
					appMessageQueue.push({'message': {'endpoint': endpoint, 'summary': true, 'title': 'Invalid response!'}});
				}
			} else {
				console.log('Request returned error code ' + xhr.status.toString());
				appMessageQueue.push({'message': {'endpoint': endpoint, 'summary': true, 'title': 'HTTP/' + xhr.statusText}});
			}
		}
		sendAppMessageQueue();
	}
	xhr.ontimeout = function() {
		console.log('HTTP request timed out');
		appMessageQueue.push({'message': {'endpoint': endpoint, 'summary': true, 'title': 'Request timed out!'}});
		sendAppMessageQueue();
	};
	xhr.onerror = function() {
		console.log('HTTP request return error');
		appMessageQueue.push({'message': {'endpoint': endpoint, 'summary': true, 'title': 'Failed to connect!'}});
		sendAppMessageQueue();
	};
	xhr.send(null);
}

Pebble.addEventListener('ready', function(e) {});

Pebble.addEventListener('appmessage', function(e) {
	console.log('AppMessage received from Pebble: ' + JSON.stringify(e.payload));
	if (e.payload.summary) {
		clipped(stories[e.payload.index].url, e.payload.endpoint);
	} else if (typeof(e.payload.endpoint) != 'undefined') {
		hackernews(e.payload.endpoint);
	} else {
		appMessageQueue = [];
	}
});

