.pragma library

// Messages and Email read the text from "status", the other targets from "data".
function textResource(text, name) {
    return { "type": "text/plain", "data": text, "status": text, "name": name }
}
