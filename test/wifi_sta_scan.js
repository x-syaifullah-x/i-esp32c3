fetch("http://192.168.4.1/api/wifi/sta/scan", {
  method: "POST",
  headers: {
    "Content-Type": "application/json"
  },
})
  .then(r => r.text())
  .then(console.log)
  .catch(console.error);