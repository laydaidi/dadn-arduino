String index_html = R"(
<html>
<head>
  <title> WebSockets Client</title>
  <script src='http://code.jquery.com/jquery-1.9.1.min.js'></script>
</head>
<body>
  <img id='live' src='' alt='Frame buffer error!'>
</body>
</html>
<script>
  jQuery(function($){
    if (!('WebSocket' in window)) {
      alert('Your browser does not support web sockets');
    }else{
      setup();
    }
    function setup(){
      var ws = new WebSocket("ws://server_ip:8888");
      ws.binaryType = 'arraybuffer';
      if(ws){
        ws.onopen = function(){
        }
        ws.onmessage = function(msg){
          var img = document.getElementById('live');
          var bytes = new Uint8Array(msg.data);
          var binary= '';
          var len = bytes.length;
          for (var i = 0; i < len; i++) {
            binary += String.fromCharCode(bytes[i])
          }
          var json = JSON.parse(binary);
          console.log(json);
          var frame = json["data"];
          img.src = 'data:image/jpg;base64,'+frame;
        }
        ws.onclose = function(){
          showServerResponse('The connection has been closed.');
        }
      }
    }
  });
</script>
)";
