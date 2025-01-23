var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onLoad);

const checkboxClass = document.querySelectorAll('.chkbox');

// Handle Top Navigation Bar
function handleNavBar() {
  var x = document.getElementById("myTopnav");
  if (x.className === "topnav") {
    x.className += " responsive";
  } 
  else {
    x.className = "topnav";
  }
}

//-------------------------------------------------------------------
//
function dataChangeFunction(x)
{
   console.log("dataChangeFunction");
//   console.log(x);
   console.log(x.name, x.value);
   if (x.name == "isnumber")
   {
      console.log("isnumber");
      if (isNaN(x.value))
      {
         alert("Enter numbers only.");
         x.value = "";
      }
      else
      {
         if (parseFloat(x.value) > parseFloat(x.max))
         {
            x.value = x.max;
         }
         if (parseFloat(x.value) < parseFloat(x.min))
         {
            x.value = x.min;
         }

         let t = [x.name, x.id, x.value];
         console.log(t);
         websocket.send(t);
      }
   }
   else if (x.name == "istext")
   {
      console.log("istext");
//      let upperCase = x.value.toUpperCase();
//      x.value = upperCase;
//      console.log(x.value);
      let t = [x.name, x.id, x.value];
      websocket.send(t);
   }
}

//-------------------------------------------------------------------
// Action the page load.
//
function onLoad(event) 
{
   initWebSocket();
}

//-------------------------------------------------------------------
//
function initWebSocket() 
{
   console.log('Opening a WebSocket connection...');
   websocket = new WebSocket(gateway);
   websocket.onopen    = onOpen;     // connection opened
   websocket.onclose   = onClose;    // connection closed
   websocket.onmessage = onMessage;  // where messages will go
}

//-------------------------------------------------------------------
//
function onOpen(event) 
{
   console.log('Connection opened');
   websocket.send("states");
}
  
//-------------------------------------------------------------------
//
function onClose(event) 
{
   console.log('Connection closed');
   setTimeout(initWebSocket, 2000);
} 

//-------------------------------------------------------------------
//
function onMessage(event) 
{
   var obj = JSON.parse(event.data);  // save JSON data

   console.log("onMessage");
   console.log(obj);

   // Cycle through the checkboxClass to find matches.
   for (let element of checkboxClass)
   {
      // The obj array contains the Button[] array as defined in Main
      // (created by GetButtonStates in CControl). The checkboxClass 
      // defined above contains only the list of the buttons defined 
      // in this html document. They therefore may not be the same.
      // Here we search through the number of Button array for a matching 
      // id to the buttons on this page.
      //
      for (i in obj.button)
      {
         var id = obj.button[i].id;     // button in database
         console.log(element.id, id);
         if (element.id == id)            // match this page button?
         {
            var checked = obj.button[i].checked;
            var onText = obj.button[i].onText;
            var offText = obj.button[i].offText;

            console.log("Match", element.id, id);
            if (checked == 1)
            {
               element.checked = true;
            }
            else
            {
               element.checked = false;
            }
            console.log(element.id, element.checked, checked);

            // This element matches. Write the correct text into
            // that element based on the checkbox state.
            if (element.checked)
            {
               document.getElementById(element.id+"k").innerHTML = offText;
               var xhr = new XMLHttpRequest();
               xhr.open("GET", "/updates?id=" + element.id + "&checked=1", true);
               xhr.send();
            }
            else 
            {
               document.getElementById(element.id+"k").innerHTML = onText;
               var xhr = new XMLHttpRequest();
               xhr.open("GET", "/updates?id=" + element.id + "&checked=0", true);
               xhr.send();
            }
         }

//         if (element.id == 12 && element.checked)
//         if (element.id == 11 && element.checked)
//         {
// //            type="text/javascript" src="chart.umd.min.js"
// //            type="text/html" src=""
// console.log("loading html");
//                window.location = "plot-slug.html";
// //console.log("loading js");               
// //               window.location = "SD/chart.umd.min.js";    //.location = "/plot-slug.html";
// //console.log("both loaded");
//          }
      }
   }

//   console.log("onMessage - done");
}

//-------------------------------------------------------------------
// This function is defined in the html as managing events
// from each checkbox. It sends the element id details to the
// server which replies with an onMessage event to update the 
// client 'k' text entities. The CSS manages the colours.
//
function toggleCheckbox(element) 
{
   console.log("toggleCheckbox");
   let t = ["checkbox", element.id, element.checked];
   console.log(t);
   websocket.send(t);
}
