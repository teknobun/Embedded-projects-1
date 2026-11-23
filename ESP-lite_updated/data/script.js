const compassSlider = document.getElementById('compassSlider'); // Get the compass slider element from the HTML page


// Check if the compassSlider element exists and reset it to 0
if (compassSlider) {
	const middleValue = 0;
	compassSlider.value = middleValue;	// Set slider to the middle position (0 degrees)
	updateCompass(middleValue);			// Set the compass value to initial value of 0	
}

function findnorth() {
	fetch(`/findnorth?value=0`);

    if (compassSlider) {
        compassSlider.value = 0;
        updateCompass(0);
    }
}

function findwest() {
	fetch(`/findwest?value=270`);

    if (compassSlider) {
        compassSlider.value = 270;
        updateCompass(270);
    }
}

function findeast() {
	fetch(`/findeast?value=90`);

    if (compassSlider) {
        compassSlider.value = 90;
        updateCompass(90);
    }
}

function findsouth() {
	fetch(`/findsouth?value=180`);

    if (compassSlider) {
        compassSlider.value = 180;
        updateCompass(180);
    }
}

function sendCompassValue(pos) {
	fetch(`/compass?value=${pos}`);
}
// Function to update the compass display (show the current compass position)
function updateCompass(pos) {
	document.getElementById('compassValue').innerText = `${pos}°`;// Update the text value for compassValue when value is changing on page element with the current position and add "°" for degrees
}


// Function to send the current compass value to the server
function sendCompassValue(pos) {
	fetch(`/compass?value=${pos}`);		// Send the compass value to the server using a fetch request
	console.log("Compass value", pos);	// Log the compass value to the console for debugging
}


// Functions for moving the motor forward and backward with specific distances
function forwards10() { move('forwards',10); } 		// Calling move function with separated parameters
function forwards50() { move('forwards', 50); }		// Calling move function with separated parameters
function backwards10() { move('backwards', 10); }		// Calling move function with separated parameters
function backwards50() { move('backwards', 50);}	// Calling move function with separated parameters

// This is a general-purpose function that can handle different combinations of direction and distance
function move(dir, dis) { 
	fetch(`/${dir}${dis}`);			// Sends a request to the server with a URL constructed from the received direction and distance (e.g., /forwards5)
	console.log("Drive", dir, dis);	// Log the movement command to the console for debugging
}

function setSpeed(pwm) {
	fetch(`/setSpeed?value=${pwm}`);
}

