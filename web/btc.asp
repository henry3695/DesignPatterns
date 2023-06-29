<%
dim data
data=Application("ip")

dim flag
flag="0"
dim ip
ip=Request.ServerVariables("REMOTE_ADDR")

if IsArray(data)=true then

For i=0 To UBound(data)	
  if ip=data(i) then
	flag="ok"
	exit for
  end if 
Next

end if

if flag<>"ok" then
	Response.Status = "404 Not Found"
	'Response.write "404 Not Found"
end if
%>
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8"> 
<title>test</title> 
<style>

*{
	margin: 0;
	padding: 0;
	top: 0;
	background-color: #126E51;
}
#main
{
	position:absolute;
	width:100%;
	height:100%;
	border:1px solid red;
}

#title
{
	position:absolute;
	left:0px;
	top:0px;
	width:100%;	
	height:60px;	
	/*border:1px solid blue;*/
}
#time
{
	float:right;
	width:40%;	
	color:rgb(255,255,255);
	color: yellow;
	font-weight: 900;
	font-size: 36px;
	/*border:1px solid red;*/	
	line-height:60px;	
}

#price
{
	margin-left:10px;
	float:left;
	width:20%;	
	color:rgb(255,255,255);
	color: yellow;
	font-weight: 900;
	font-size: 36px;
	/*border:1px solid red;*/
	line-height:60px;	
}

#Ray
{	
	position:absolute;
	top:60px;
	width:100%;	
	height:60px;
	border:1px solid red;
	display:inline;
}

#d{
	width:30%;	
	height:60px;
	display:inline;
	line-height:60px;
	color:blue;
	font-weight: 900;
	font-size: 36px;	
	float:left;
	text-align:left;	
	background-color: #08c4bb;
}

#k{
	width:70%;	
	height:60px;
	display:inline;
	line-height:60px;
	color:#330a36;
	font-weight: 900;
	font-size: 36px;
	float:right;
	text-align:right;	
	background-color: #db5ac4;
}

#overflowTest {    
	position:absolute;
	top:120px;
	color:white;
    width:100%;
    height: 100px;
    overflow: scroll;
	margin-left:5px;
}
p{
line-height:28px;
font-size:26px;
}	
</style>
<script src="/jquery-3.7.0.min.js"></script>


<script>
/*
function refreshTime()
{
	var date = new Date();
	$("#time").text(date.toLocaleString());
	
	
	var rnd = Math.floor(Math.random()*10);
	
	var d = (rnd*10)+"%";
	var k = ((10-rnd)*10)+"%";
	
	$("#d").css("width",d);
	$("#k").css("width",k);
	
	$("#d").text(rnd*10);
	$("#k").text( (10-rnd)*10);
	
	console.log($(window).height());
	
	$("#overflowTest").append("<p>这里的文本内容是可以滚动的，滚动条方向是垂直方向。</p>"+rnd);
	
	var height=$("#overflowTest")[0].scrollHeight;
	$("#overflowTest").scrollTop(height);
}

$(function(){
    setInterval(refreshTime,1000);
	$("#overflowTest").css("height",($(window).height()-60-60)+"px");
	
	
	window.addEventListener("onorientationchange" in window ? "orientationchange" : "resize", function() {
        if (window.orientation === 180 || window.orientation === 0) {
            console.log("竖屏");
			$("#overflowTest").css("height",($(window).height()-60-60)+"px");
        }
        if (window.orientation === 90 || window.orientation === -90 ){
            console.log("横屏");
			$("#overflowTest").css("height",($(window).height()-60-60)+"px");
        }
    }, false);
	

});
*/

		var wsServer = 'ws://www.online100.top:7001';
		var limitConnect = 100;  // 断线重连次数
		var timeConnect = 0;
		webSocketInit(wsServer);		
		var nLine=0;
		function debug(message) 
		{
			//console.log(message);
			nLine++;
			if(nLine>100)
			{
				nLine=0;
				$("#overflowTest").text("");
			}
			$("#overflowTest").append("<p>"+message+"</p>");
			var height=$("#overflowTest")[0].scrollHeight;
			$("#overflowTest").scrollTop(height);
		}
		
		//socket初始化
		function webSocketInit(service) {
			var ws = new WebSocket(service);
			//ws.binaryType = 'arraybuffer'
			ws.onopen = function () {
				console.log("已连接TCP服务器");
			};
			ws.onmessage = function (msg) {						
				var obj = JSON.parse(msg.data);
				if(obj.cmd==1)
				{
					var buy=Math.round(obj.Buy30 * 100 / (obj.Sell30+obj.Buy30));
					var d = buy+"%";
					var k = (100-buy)+"%";
					
					$("#d").css("width",d);
					$("#k").css("width",k);
					
					$("#d").text(obj.str_Buy30);
					$("#k").text(obj.str_Sell30);	

					$("#price").text(obj.s_price);					
				}
				else if(obj.cmd==2)
				{
					debug(obj.str_msg);
				}	
				
				var date = new Date();
				$("#time").text(date.toLocaleString());				

			};
			ws.onclose = function () {
				console.log('服务器已经断开');
				reconnect(service);
			};

			// 重连
			function reconnect(service) {
				// lockReconnect加锁，防止onclose、onerror两次重连
				if (limitConnect > 0) {
					limitConnect--;
					timeConnect++;
					console.log("第" + timeConnect + "次重连");
					// 进行重连
					setTimeout(function () {
						webSocketInit(service);
					}, 1000);

				} else {
					console.log("TCP连接已超时");
				}
			};
		}


		$(function () {			
			$("#overflowTest").css("height",($(window).height()-60-60)+"px");
	
	
			window.addEventListener("onorientationchange" in window ? "orientationchange" : "resize", function() {
				if (window.orientation === 180 || window.orientation === 0) {
					console.log("竖屏");
					$("#overflowTest").css("height",($(window).height()-60-60)+"px");
				}
				if (window.orientation === 90 || window.orientation === -90 ){
					console.log("横屏");
					$("#overflowTest").css("height",($(window).height()-60-60)+"px");
				}
			}, false);
	
		});

</script>
</head>
<body>
<div id="main">
<div id="title">
<span id="time">2023/6/14 13:30:53</span>
<span id="price">30000.10</span>
</div>

<div id="Ray">
<div id="d">30</div> 
<div id="k">70</div> 
</div>

<div id="overflowTest"></div>
</div>
</body>
</html>
