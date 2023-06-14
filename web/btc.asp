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
<html>

<head>
	<title>BTCUSDT 合约</title>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	<!--
	<link rel="stylesheet" href="http://code.jquery.com/ui/1.10.4/themes/smoothness/jquery-ui.css">
	<script src="http://code.jquery.com/jquery-1.9.1.js"></script>
	<script src="http://code.jquery.com/ui/1.10.4/jquery-ui.js"></script>
	<link rel="stylesheet" href="http://jqueryui.com/resources/demos/style.css">
	<script src="jquery.cookie.js"></script>
	<script src="https://cdn.bootcdn.net/ajax/libs/pako/2.0.3/pako.min.js"></script>
	-->
	
	<link rel="stylesheet" href="jquery-ui.css">
	<script src="jquery-1.9.1.js"></script>
	<script src="jquery-ui.js"></script>
	<link rel="stylesheet" href="style.css">
	<script src="jquery.cookie.js"></script>
	<script src="pako.min.js"></script>
	<style>
		* {
			margin: 0;
			padding: 0;
			top: 0;
		}

		#main {
			text-align: left;
			width: 980px;
			height: 800px;
			border: 1px;
		}

		body {
			background-color: #126E51;
		}

		span.num {
			color: yellow;
			font-weight: 900;
			font-size: 350px;
		}

		span.time {
			color: yellow;
			font-weight: 900;
			font-size: 36px;
		}

		#top {
			float: left;
			clear: left;
			filter: alpha(opacity=60);
			background-color: #777;
			z-index: 1002;
			left: 0px;
			opacity: 0.5;
			-moz-opacity: 0.5;
			width: 100%;
		}

		#progressBar {
			float: left;
			clear: left;
			background-color: yellow;
			border-style: solid;
			border-width: 1px;
			border-color: #000;
			height: 20px;
			width: 100%;
		}

		#btc {
			float: left;
			clear: left;
		}

		#div_profit {
		left-margin:10px;
			float: left;
			clear: left;
			background-color: #ccddff;
			border-style: solid;
			border-width: 1px;
			border-color: #000;
			height: 60px;
			line-height: 60px;
			width: 100%;
			color: #000;
			font-weight: 900;
			font-size: 36px;
		}
		#div_msg {
			float: left;
			clear: left;
			background-color: #ccddff;
			border-style: solid;
			border-width: 1px;
			border-color: #000;
			height: 60px;
			line-height: 60px;
			width: 100%;
			color: #000;
			font-weight: 900;
			font-size: 36px;
		}

		#div_fanan {
			float: left;
			clear: left;
			background-color: #ccddff;
			border-style: solid;
			border-width: 1px;
			border-color: #000;
			height: 950px;
			line-height: 45px;
			width: 100%;
			color: #000;
			font-weight: 350;
			font-size: 32px;
		}

		#div_btns {
			background-color: #126E51;
			width: 550px;
			float: right;
			margin-top: 2px;
		}

		#div_btns button {
			width: 80px;
			height: 25px;
			background-color: gray;
			font-size: 16px;
			color: white;
			zoom: 200%;
		}

		#div_btns .active {
			background-color: orange;
			color: blue;
		}
		textarea {
			font-size: 1.2rem;
			letter-spacing: 0px;
		}

		textarea {
			padding: 10px;
			max-width: 100%;
			line-height: 1.5;
			border-radius: 5px;
			border: 1px solid #ccc;
			box-shadow: 1px 1px 1px #999;
		}

		label {
			display: block;
			margin-bottom: 10px;
		}

	</style>


	<script>
		var wsServer = 'ws://www.online100.top:7001';
		var limitConnect = 100;  // 断线重连次数
		var timeConnect = 0;
		webSocketInit(wsServer);
		function showTime()
		{
			//var today = new Date();
			//console.log("The time is: " + today.toString ());	
		}

		var nLine=0;
		function debug(message) 
		{
			//console.log(message);
			nLine++;
			if(nLine>100)
			{
				nLine=0;
				$("#debugTextArea").val(message + "\n");
			}
			else
			{
				$("#debugTextArea").val( $("#debugTextArea").val()+message + "\n");
			}
			var height=$("#debugTextArea")[0].scrollHeight;
			$("#debugTextArea").scrollTop(height);
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
					//debug(obj.s_price+","+obj.MBuy30+","+obj.MSell30);
					
					var font_color="blue";
					if(obj.Buy30>obj.Sell30)
					{
						font_color="blue";
					}
					else
					{
						font_color="red";
					}
					$("#div_profit").html("<font color='yellow'>"+obj.s_price+"</font>    <font color="+font_color+">" + obj.str_Buy30 + "   VS   " + obj.str_Sell30+ "</font>  ");
					//$("#sp1").text(obj.s_price);
					
					//console.log(obj);
					
					
					$("#progressBar").css({"background": ""+font_color+""});
					if( (obj.Sell30+obj.Buy30)>0)
					{
						$("#progressBar").progressbar({value: Math.round(obj.Buy30 * 100 / (obj.Sell30+obj.Buy30) ) });
					}				
				}
				else if(obj.cmd==2)
				{
					debug(obj.str_msg);
				}	
				
				var date = new Date();
				$("#sp2").text(date.toLocaleString());
				/*$("#progressBar").progressbar({
					value: Math.round(date.getSeconds() * 100 / 60)
				});

				$("#progressBar").css({
					"background": '#' + Math.floor(Math.random() * 16777215).toString(16)
				});*/

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

	
		});

	</script>



</head>

<body>
	<div id="main">
		<div id="top" class="top">
			<span class="time" id="sp2">last time:</span>
			<div id="div_btns">
				<button id="btn_profit">隐藏收益</button>
				<button id="btn_input">填写数据</button>
				<button id="btn_fanan">显示方案</button>
			</div>
		</div>
		<div id="progressBar"></div>
		<div id="div_profit"></div>	
		<div id="div_msg"><textarea id="debugTextArea"  rows="20" cols="137"></textarea></div>		
	</div>


</body>

</html>
