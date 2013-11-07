
function iconFromWeatherId(weatherId) {
  if (weatherId >= 802) {
    return 0;
  } else if (weatherId >= 801) {
    return 1;
  } else if (weatherId >= 800) {
    return 2;
  } else if (weatherId >= 700) {
    return 3;
  } else if (weatherId >= 600) {
    return 4;
  } else if (weatherId >= 300) {
    return 5;
  } else {
    return 6;
  }
}

function fetchWeather(latitude, longitude) {
  var response;
  var req = new XMLHttpRequest();
  req.open('GET', "http://api.openweathermap.org/data/2.5/weather?" +
    "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if (req.status == 200) {
        console.log(req.responseText);
        response = JSON.parse(req.responseText);
        var temperature, icon, city;
        if (response) {
          temperature = Math.round((response.main.temp - 273.15) * 1.8 + 32);
          icon = iconFromWeatherId(response.weather[0].id);
          city = response.name;
          console.log("temp " + temperature);
          console.log("icon " + icon);
          console.log("city " + city);
          Pebble.sendAppMessage({
            "icon":icon,
            "temperature":temperature + "\u00B0F",
            "city":city});
        }
      } else {
        console.log("Error");
      }
    }
  }
  req.send(null);
}

function locationSuccess(pos) {
  var coordinates = pos.coords;
  fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({
    "icon":7,
    "temperature":"N/A",
    "city":"Loc Unavailable"
  });
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 };

Pebble.addEventListener("ready",
                        function(e) {
                          console.log("connect!" + e.ready);
                          window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
                          console.log(e.type);
                        });

Pebble.addEventListener("appmessage",
                        function(e) {
                          window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
                          console.log(e.type);
                          console.log(e.payload.temperature);
                          console.log("message!");
                        });
