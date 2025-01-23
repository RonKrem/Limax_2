var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

// These constants must match both the names defined in about.html
// and here where they are updated via the getValues function. ie:
// 1. <input type="text", id="p1_mins", name="isnumber", min="0", 
//    max="60", onchange="dataChangeFunction(this)">
// 2. document.getElementById(PERIOD1_MINS).value = obj.p1_mins;
// The names in the server must match internally also, but may have
// different names in the client code. For example the getElement code 
// could be:
//  document.getElementById(PERIOD1_MINS).value = obj.period1_mins;
// as long as the period1_mins parameter was the same elsewhere in
// the server.
//
// +++ THESE MUST MATCH THE LIST IN MAIN.CPP +++
//

// Buttons.
//

//-------------------------------------------------------------------
//
function dataChangeFunction(x)
{
   console.log("dataChangeFunction");
   console.log(x);
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
   else if ( (x.name == "istext") || (x.name == "isheader") )
   {
      console.log("istext");
//      let upperCase = x.value.toUpperCase();
//      x.value = upperCase;
//      console.log(x.value);
      let t = [x.name, x.id, x.value];
      websocket.send(t);
   }
}
