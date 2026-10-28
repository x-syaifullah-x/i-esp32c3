const ws = new WebSocket("ws://192.168.4.1")

ws.onclose = (e) => {
  console.log("Server closed:", e)
}

ws.onopen = (e) => {
  ws.send(
    JSON.stringify({
      token: "TOKEN_A",
    }),
  )
}

ws.onmessage = (e) => {
  console.log("Message from server:", e.data)
}

// WIFI_AP_STATUS
ws.send(
  JSON.stringify({
    type: 0,
    action: 0,
  }),
)


// EXAMPLE TYPE ARRAYBUFFER
ws.binaryType = "arraybuffer"
ws.send(
  new Uint8Array([0, 1, 2, 3, 4, 5, 6, 7])
)