// Firebase configuration
var firebaseConfig = {
  apiKey: "AIzaSyDrg4FCw5K-4Dn40HG9kVHbmm90giZsmVU",
  authDomain: "coal-project-5e6db.firebaseapp.com",
  databaseURL: "https://coal-project-5e6db-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "coal-project-5e6db",
  storageBucket: "coal-project-5e6db.appspot.com",
  messagingSenderId: "726696828468",
  appId: "1:726696828468:web:3eaf4d8b3c123456789abc"
};

// Initialize Firebase
firebase.initializeApp(firebaseConfig);
var database = firebase.database();

// Fetch data from Firebase
function fetchData() {
  var blocksRef = database.ref('/blocks');
  blocksRef.on('value', (snapshot) => {
      var data = snapshot.val();
      updateBlock('block1', data.block1);
      updateBlock('block2', data.block2);
      updateBlock('block3', data.block3);
      updateBlock('block4', data.block4);
  });
}

// Update block data
function updateBlock(blockId, blockData) {
  var blockIndicator = document.getElementById(`${blockId}-indicator`);
  var nodesContainer = document.getElementById(`nodes-${blockId}`);
  nodesContainer.innerHTML = '';

  var blockHasAlarm = false;

  for (var nodeId in blockData) {
      var nodeData = blockData[nodeId];
      var nodeDiv = document.createElement('div');
      nodeDiv.classList.add('node');

      var buzzerState = nodeData.buzzer ? 'ON' : 'OFF';
      nodeDiv.innerHTML = `
          <h3>${nodeId}</h3>
          <p>Temperature: ${nodeData.temp} °C</p>
          <p>Humidity: ${nodeData.humi} %</p>
          <p>Gas: ${nodeData.gas}</p>
          <p>Buzzer: ${buzzerState}</p>
      `;

      if (nodeData.buzzer) {
          blockHasAlarm = true;
      }

      nodesContainer.appendChild(nodeDiv);
  }

  blockIndicator.style.backgroundColor = blockHasAlarm ? 'red' : 'green';
}

// Toggle nodes display when a block is clicked
function toggleNodeVisibility(blockId) {
  var nodes = document.getElementById(`nodes-${blockId}`);
  nodes.style.display = nodes.style.display === 'none' ? 'block' : 'none';
}

// Add event listeners for each block
document.getElementById('block1').addEventListener('click', function() {
  toggleNodeVisibility('block1');
});

document.getElementById('block2').addEventListener('click', function() {
  toggleNodeVisibility('block2');
});

document.getElementById('block3').addEventListener('click', function() {
  toggleNodeVisibility('block3');
});

document.getElementById('block4').addEventListener('click', function() {
  toggleNodeVisibility('block4');
});

// Initialize
fetchData();
