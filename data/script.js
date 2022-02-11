var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

var loglist = "";
const dataMap = new Map();

/**
 * Set up websocket and define callback methods
 */
function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage; // <-- add this line
}


function onOpen(event) {
  console.log('Connection opened');
}

function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  //Update internal data structures
  const input = JSON.parse(event.data);
  input.combined.forEach(msg => {
    loglist += "<p>" + msg.id + ", type: " + msg.type + ", length: " + msg.length + ", data: " + msg.data + " (RTR length: " + msg.rtrlength + ")</p>";
    addData(msg);
  });

  //insert it in the html at the correct location
  document.getElementById('log').innerHTML = loglist;
  document.getElementById('statistics').innerHTML = mapToTable2(dataMap).outerHTML;
  scrollLog();
}

/**
 * Add the incoming messages to the hashmap registering all messages & payloads
 * @param  data 
 */
function addData(data) {
  let idMap;
  if (dataMap.has(data.id)) {
    idMap = dataMap.get(data.id);
  } else {
    idMap = new Map();
    dataMap.set(data.id, idMap);
  }
  let pdMap;
  if (idMap.has(data.data)) {
    pdMap = idMap.get(data.data);
  } else {
    pdMap = new Map();
    idMap.set(data.data, 0);
  }
  const count = idMap.get(data.data);
  idMap.set(data.data, count + 1);
}

/**
 * transform the data in the map to a bunch of nested tables for visualization
 * @param {*} map 
 * @returns 
 */
function mapToTable(map) {
  const table = document.createElement('table');
  //get them sorted so it's easier to find something
  let keys = Array.from(map.keys()).sort();

  //Iterate over all items
  keys.forEach(key => {
    const row = document.createElement('tr');
    const cell1 = document.createElement('td');
    const cell2 = document.createElement('td');
    cell1.appendChild(document.createTextNode(key));
    const temp = map.get(key);
    //We're using nested maps, so just call this method recursively
    if (temp instanceof Map) {
      cell2.appendChild(mapToTable(temp));
    } else {
      cell2.appendChild(document.createTextNode(temp));
    }
    row.appendChild(cell1);
    row.appendChild(cell2);
    table.appendChild(row);
  });
  return table;
}

function mapToTable2(map) {
  const table = document.createElement('table');
  let keys = Array.from(map.keys()).sort();
  keys.forEach(key => {
    const res = addTableSection(key, map.get(key));
    table.appendChild(res.child);
  });
  return table;
}

/**
 * Issue with this function: it creates a row too much, compensated with a rowspan of +1
 * not really perfect, but it works I guess
 * 
 * @param {*} key 
 * @param {*} map 
 * @returns 
 */
function addTableSection(key, map) {
  const row = document.createElement('tr');
  const cell1 = document.createElement('td');
  cell1.appendChild(document.createTextNode(key));
  row.appendChild(cell1);
  let rowspan = 1;
  if (map instanceof Map) {
    const keys = Array.from(map.keys()).sort();
    let first=true;
    //do the recursion thing
    keys.forEach(k => {
      const res = addTableSection(k, map.get(k));
      rowspan += res.size;
      row.appendChild(res.child);
    });
 
  } else {
    //Last part
    const cell2 = document.createElement('td');
    cell2.appendChild(document.createTextNode(map));
    row.appendChild(cell2);
    rowspan = 1;
  }
  attrib = document.createAttribute("rowspan");
  attrib.value = rowspan;
  cell1.setAttributeNode(attrib);
  return { "size": rowspan, "child": row };
}

window.addEventListener('load', onLoad);

function onLoad(event) {
  initWebSocket();
  //initButton();
}

/**
 * In the scrollable field keep it always on the last line
 */
function scrollLog() {
  var objDiv = document.getElementById("log");
  objDiv.scrollTop = objDiv.scrollHeight;
}

/*
function initButton() {
  document.getElementById('button').addEventListener('click', toggle);
}*/
