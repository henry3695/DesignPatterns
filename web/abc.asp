
<%
dim data
data=Application("ip")

if IsArray(data)=false then
dim a
a=Array(Request.ServerVariables("REMOTE_ADDR"))
For i=0 To UBound(a)
  response.write("a("&i&")="&a(i)&"<br>")
Next
Application("ip")=a
response.write "refresh"
else
For i=0 To UBound(data)
	response.write("data("&i&")="&data(i)&"<br>")
  if Request.ServerVariables("REMOTE_ADDR")=data(i) then
	Response.end
  end if 
Next

n = UBound(data)+1
dim b()
redim b(n)
For i=0 To UBound(data)
  b(i)=data(i)
Next
b(n)=Request.ServerVariables("REMOTE_ADDR")
Application("ip")=b
end if


%>
