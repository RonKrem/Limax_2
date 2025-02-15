   // Get current sensor readings when the page loads  
   var gateway = `ws://${window.location.hostname}/ws`;
   var websocket;


   const TEST_ELAPSED_TIME    =  "etime"
   const STEP_SIZE            =  "stp_siz"

   const SENSOR_1_PRESS       =  "press1"
   const SENSOR_1_PRESS_UNIT  =  "p1unit"
   const SENSOR_1_TEMP        =  "temp1"
   const SENSOR_1_TEMP_UNIT   =  "t1unit"

   const SENSOR_2_PRESS       =  "press2"
   const SENSOR_2_PRESS_UNIT  =  "p2unit"
   const SENSOR_2_TEMP        =  "temp2"
   const SENSOR_2_TEMP_UNIT   =  "t2unit"

   const SENSOR_3_PRESS       =  "press3"
   const SENSOR_3_PRESS_UNIT  =  "p3unit"
   const SENSOR_3_TEMP        =  "temp3"
   const SENSOR_3_TEMP_UNIT   =  "t3unit"

   window.addEventListener('load', onLoad);
   window.addEventListener('load', getReadings);

   var maxDataPoints = 30;


   //-------------------------------------------------------------------
   //
   function  onLoad(event) {
      initWebSocket();
//      initButtons();
   }

   //-------------------------------------------------------------------
   //
   function initWebSocket() {
      console.log('Trying to open a WebSocket connection…');
      websocket = new WebSocket(gateway);
      websocket.onopen    = onOpen;
      websocket.onclose   = onClose;
   }

   //-------------------------------------------------------------------
   //
   function onOpen(event) {
      console.log('Connection opened');
   }

   //-------------------------------------------------------------------
   //
   function onClose(event) {
      console.log('Connection closed');
      setTimeout(initWebSocket, 2000);
   }

   //-----------------------------------------------------------------------------
   function setMinAxis(minValue) 
   {
      myChart0.options.scales.y.min = Math.trunc(minValue);
   }

   //-----------------------------------------------------------------------------
   function removeOldData(chart) 
   {
      chart.data.labels.shift();
      chart.data.datasets.forEach((dataset) => {
         dataset.data.shift();
      });
   }

   //-----------------------------------------------------------------------------
   function abortPlot()
   {
      removeOldData(myChart0);
      removeOldData(myChart1);
      removeOldData(myChart2);
      removeOldData(myChart3);
      myChart0.destroy();
      myChart1.destroy();
      myChart2.destroy();
      myChart3.destroy();
      console.log("destroyed");
   }

      //-----------------------------------------------------------------------------
      function setMinAxis(minValue) 
      {
         myChart0.options.scales.y.min = Math.trunc(minValue);
      }

   //-----------------------------------------------------------------------------
   function addData(chart, label, data)
   {
      if (chart.data.labels.length > maxDataPoints) 
      {
         removeOldData(chart);
      }
      chart.data.labels.push(label);
      chart.data.datasets.forEach((dataset) => {
         dataset.data.push(data);
      });
      chart.update();        
   }

   //-----------------------------------------------------------------------------
   function deleteData(chart)
   {
      chart.data.labels.pop();
      chart.data.datasets.forEach((dataset) => {
         dataset.data.pop();
      });
   }

   //-----------------------------------------------------------------------------
   // Create Position Chart
   const ctx0 = document.getElementById('myChart0').getContext('2d');
   const plotData0 = 
   {
      labels: [],
      datasets: [
         {
            data: [],
            borderColor: 'red',
            borderWidth: 2,
            tension: 0,
            fill: false,
         }],
      };
      
   const myChart0 = new Chart(ctx0, 
   {
      type: 'line',
      data: plotData0,
      options: 
      {
         plugins: 
         {
            legend: 
            {
               display: false
            },
            title: 
            {
               display: true,
               text: 'SLUG DEPTH PROFILE',
               color: 'navy',
               font: 
               {
                  size: 20
               }
            }
         },
         scales: 
         {
            y: 
            {
               title: 
               {
                  display: true,
                  padding: 5,
                  text: 'Slug Depth Travel (m)',
                  color: 'navy',
                  font: 
                  {
                     size: 12
                  }
               }
            }
         },
         elements: 
         {
            point: 
            {
               radius: 0
            }
         },
         responsive: true,
         maintainAspectRatio: false,
      }
   });

   //-----------------------------------------------------------------------------
   const ctx1 = document.getElementById('myChart1').getContext('2d');
   const plotData1 = {
      labels: [],
      datasets: [
         {
            data: [],
            borderColor: 'blue',
            borderWidth: 2,
            tension: 0,
            fill: false
         }],
      };
      
   const myChart1 = new Chart(ctx1, 
   {
      type: 'line',
      data: plotData1,
      options: 
      {
         plugins: 
         {
            legend: 
            {
               display: false
            },
            title: 
            {
               display: true,
               text: 'DRIVING WELL SENSOR',
               color: 'navy',
               font: 
               {
                  size: 20
               }
            }
         },
         scales: 
         {
            y: 
            {
               title: 
               {
                  display: true,
                  padding: 5,
                  text: 'Sensor 1 Pressure (cmH20)',
                  color: 'navy',
                  font: 
                  {
                     size: 12
                  }
               }
            }
         },
         elements: 
         {
            point: 
            {
               radius: 0
            }
         },
         responsive: true,
         maintainAspectRatio: false,
      }
   });

   //-----------------------------------------------------------------------------
   const ctx2 = document.getElementById('myChart2').getContext('2d');
   const plotData2 = {
      labels: [],
      datasets: [
         {
            data: [],
            borderColor: 'blue',
            borderWidth: 2,
            fill: true,
            tension: 0,
            fill: false
         }],
      };

      
   const myChart2 = new Chart(ctx2, 
   {
      type: 'line',
      data: plotData2,
      options: 
      {
         plugins: 
         {
            legend: 
            {
               display: false
            },
            title: 
            {
               display: true,
               text: 'OBSERVATION WELL 1',
               color: 'navy',
               font: 
               {
                  size: 20
               }
            }
         },
         scales: 
         {
            y: 
            {
               title: 
               {
                  display: true,
                  padding: 5,
                  text: 'Sensor 2 Pressure (cmH20)',
                  color: 'navy',
                  font: 
                  {
                     size: 12
                  }
               }
            }
         },
         elements: 
         {
            point: 
            {
               radius: 0
            }
         },
         responsive: true,
         maintainAspectRatio: false,
      }
   });

   //-----------------------------------------------------------------------------
   const ctx3 = document.getElementById('myChart3').getContext('2d');
   const plotData3 = {
      labels: [],
      datasets: [
         {
            data: [],
            borderColor: 'blue',
            borderWidth: 2,
            tension: 0,
            fill: false
         }],
      };
      
   const myChart3 = new Chart(ctx3, 
   {
      type: 'line',
      data: plotData3,
      options: 
      {
         plugins: 
         {
            legend: 
            {
               display: false
            },
            title: 
            {
               display: true,
               text: 'OBSERVATION WELL 2',
               color: 'navy',
               font: 
               {
                  size: 20
               }
            }
         },
         scales: 
         {
            y: 
            {
               title: 
               {
                  display: true,
                  padding: 5,
                  text: 'Sensor 3 Pressure (cmH20)',
                  color: 'navy',
                  font: 
                  {
                     size: 12
                  }
               }
            }
         },
         elements: 
         {
            point: 
            {
               radius: 0
            }
         },
         responsive: true,
         maintainAspectRatio: false,
      }
   });
   
   myChart0.canvas.parentNode.style.height = '250px';
   myChart1.canvas.parentNode.style.height = '250px';
   myChart2.canvas.parentNode.style.height = '250px';
   myChart3.canvas.parentNode.style.height = '250px';
   
   //-----------------------------------------------------------------------------
   // Plot the current slug position and three sensor readings
   // on the four charts. 
   function Plot(time, slug, sensor1, sensor2, sensor3) 
   {
      console.log("Plot");
//      console.log(slug);
//         var today = new Date();
//         var t = today.getHours() + ":" + today.getMinutes() + ":" + today.getSeconds();
      addData(myChart0, time, slug);
      addData(myChart1, time, sensor1);
      addData(myChart2, time, sensor2);
      addData(myChart3, time, sensor3);
   }

   //-----------------------------------------------------------------------------
   // Function to get current readings from the input fields in response to the
   // webpage being loaded or refreshed.
   //
   function getReadings() 
   {
      var xhr = new XMLHttpRequest();

      console.log("getPlotValues");
      xhr.onreadystatechange = function () 
      {
         if (this.readyState == 4 && this.status == 200)
         {
//            console.log(this.responseText);
            var obj = JSON.parse(this.responseText);
//            console.log(obj.plotEntries);
//            console.log(obj);
            for (let i=0; i < obj.plotEntries - 1; i++)
            {
               let entry = "dataset" + i;
               let myindex = "etime";
//               console.log(entry);
//               console.log(obj[entry]);
               var line = JSON.parse(obj[entry]);
//               console.log(line);
               Plot(line.etime, line.sdepth, line.press1, line.press2, line.press3);
            }
         }
      };
      xhr.open("GET", "/readings", true);
      xhr.send();
   }

   //-----------------------------------------------------------------------------
   if (!!window.EventSource) 
   {
      var source = new EventSource('/events');
   
      //-----------------------------------------------------------------------------
      source.addEventListener('open', function(e) 
      {
         console.log("Events Connected");
      }, false);
   
      //-----------------------------------------------------------------------------
      source.addEventListener('error', function(e) 
      {
         if (e.target.readyState != EventSource.OPEN) 
         {
            console.log("Events Disconnected");
         }
      }, false);

      //-----------------------------------------------------------------------------
      source.addEventListener('message', function(e) 
      {
         // This would be all the buttons.
         //
         console.log("message");
      }, false);
      
      //-----------------------------------------------------------------------------
      source.addEventListener('new_values', function(e) 
      {
         console.log("new_values");
         var obj = JSON.parse(e.data);
//         console.log(obj);

         Plot(obj.etime, obj.sdepth, obj.press1, obj.press2, obj.press3);

         document.getElementById(TEST_ELAPSED_TIME).innerHTML     = obj.etime;

         document.getElementById(SENSOR_1_PRESS).innerHTML        = obj.press1;
         document.getElementById(SENSOR_1_PRESS_UNIT).innerHTML   = obj.p1unit;

         document.getElementById(SENSOR_2_PRESS).innerHTML        = obj.press2;
         document.getElementById(SENSOR_2_PRESS_UNIT).innerHTML   = obj.p2unit;

         document.getElementById(SENSOR_3_PRESS).innerHTML        = obj.press3;
         document.getElementById(SENSOR_3_PRESS_UNIT).innerHTML   = obj.p3unit;

      }, false);
      
      //-----------------------------------------------------------------------------
      source.addEventListener('new_readings', function(e) 
      {
         console.log("new_readings");
         var obj = JSON.parse(e.data);
         console.log(obj);

         Plot(obj.stime, obj.slug, 0, 0, 0);

      }, false);

      //-----------------------------------------------------------------------------
      // Abort function
      source.addEventListener('abort_plot', function(e) 
      {
         console.log("abort");
         abortPlot();
      }, false);

      //-----------------------------------------------------------------------------
      // Set y axis min value function
      source.addEventListener('set_min_value', function(e) 
      {
         console.log("set_min_value");
         var myObj = JSON.parse(e.data);
//         console.log(myObj);
         setMinAxis(myObj.minValue) 
      }, false);

      //-----------------------------------------------------------------------------
      // Set the Y axis text that includes the units.
      source.addEventListener("set_y_axis_text", function (e)
      {
         console.log("set_y_axis_text");
         var myObj = JSON.parse(e.data);
//         console.log(myObj);
         setYaxisText(myObj.yaxis0, myObj, )
      }, false);
   }
