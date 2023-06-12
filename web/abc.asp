<%
Application("ip")=Request.ServerVariables("REMOTE_ADDR")
Response.Write "ip:"&Application("ip")
%>