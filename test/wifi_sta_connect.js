fetch("http://192.168.4.1/api/wifi/sta/connect", {
  method: "POST",
  headers: {
    "Content-Type": "application/json"
  },
  body: JSON.stringify({
    ssid: "adibah.Net WiFi-",
    password: "",
    authmode: 0
  })
})
  .then(r => r.text())
  .then(console.log)
  .catch(console.error);