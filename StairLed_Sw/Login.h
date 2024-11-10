const char weblogin[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8"> 
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body {font-family: Time New Roman;}
.login-page {
  width: 360px;
  padding: 8% 0 0;
  margin: auto;
}
.form {
  position: relative;
  border-radius: 10px;
  z-index: 1;
  background: #FFFFFF;
  max-width: 360px;
  margin: 0 auto 100px;
  padding: 30px;
  text-align: center;
  box-shadow: 0 0 20px 0 rgba(0, 0, 0, 0.2), 0 5px 5px 0 rgba(0, 0, 0, 0.24);
  background: radial-gradient(farthest-side ellipse at 10% 0, hsl(300, 100%, 95%), hsl(190, 50%, 70%) 80%, hsl(226, 40%, 60%) 120%);
}
.form input {
  font-family: "Times New Roman", sans-serif;
  border-radius: 10px;
  outline: 0;
  background: #f2f2f2;
  width: 100%;
  border: 0;
  margin: 0 0 15px;
  padding: 15px;
  box-sizing: border-box;
  font-size: 14px;
}
.form button {
  font-family: "Times New Roman", sans-serif;
  border-radius: 10px;
  text-transform: uppercase;
  outline: 0;
  background-color: #04AA6D;
  width: 100%;
  border: 0;
  padding: 15px;
  color: #FFFFFF;
  font-size: 14px;
  -webkit-transition: all 0.3 ease;
  transition: all 0.3 ease;
  cursor: pointer;
}
.form button:hover,.form button:active,.form button:focus {
  opacity: 0.8;
}
.copyright {
    text-align: center;
    background-color: #333;
    color: #fff;
    padding: 10px;
    position: fixed;
    bottom: 0;
    left: 0;
    width: 100%;
}
body {
  background: white; /* fallback for old browsers */    
}
h1{
  font-family: "Times New Roman", sans-serif;
  font-size: 28px;
}
</style>
</head>
<body onload = "UpdateData()"> 
<div class="login-page">
  <div class="form">
      <h1 align="center">Login</h1>
      <form action="/Login" method="post" name = "login" onsubmit="return validateform_login()">
        <input type="text" placeholder="username" name = "username_login" id = "username_login">
        <input type="password" placeholder="password" name = "password_login" id = "password_login">
        <button type="submit">login</button>
      </form>
  </div>
</div>   
<div class="copyright">&copy; congchuc289@gmail.com</div> 
</body>
<script type="text/javascript">
function validateform_login() {
    var name = document.getElementById("username_login").value;
    var password = document.getElementById("password_login").value;

    if (name != "Admin") {
        alert("Enter the username");
        return false;
      }
    if(password != "Admin"){
        alert("Enter password");
        return false;
      }
    if(password == null || password  == ""){
        alert("Please enter password and username");
        return false;
      }
    else{
        alert("Successfull");
        window.location.href = "/Config";
    }
} 
function UpdateData()
{
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function ()
  {
    if(this.readyState == 4 && this.status == 200)
    {
      var DataVDK  = xhttp.responseText;
      console.log("Dữ liệu VDK:" + DataVDK);   
      var DataJson = JSON.parse(DataVDK); 
        document.getElementById("username_login").innerHTML = DataJson.username_login;
        document.getElementById("password_login").innerHTML = DataJson.password_login;   
    }
  }
  xhttp.open('GET','/sendweb',true);
  xhttp.send(); 
  setTimeout(function(){ UpdateData() }, 1000); 
}       
</script>
</html> 
)=====";
