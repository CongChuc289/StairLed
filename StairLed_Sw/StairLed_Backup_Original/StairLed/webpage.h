const char webpage[] PROGMEM = R"=====(
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
  border-radius: 20px;
  white-space: nowrap;
  position: relative;
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
  padding: 10px;
  box-sizing: border-box;
  font-size: 13px;
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
.form input::placeholder {
  font-family: "Times New Roman", Times, serif;
  font-size: 14px; /* Adjust the font size as needed */
  /* Add any additional styling for the placeholder text */
}
body {
  background: white; /* fallback for old browsers */    
}
h1{
  font-family: "Times New Roman", sans-serif;
  font-size: 28px;
}
h5{
  font-family: "Times New Roman", sans-serif;
}
</style>
</head>
<body onload="UpdateData()"> 
  <div class="login-page">
    <div class="form">    
      <h1 align="center">Config Stair LED</h1>
      <form action="/Config" method="post" name="config" onsubmit="return validateform()">
        <input type="text" placeholder="Lựa chọn chế độ" name="mode_act" id="mode_act">
        <br> 
        <br> 
        <input type="text" placeholder="Số bậc" name="number_step" id="number_step" >   
        <br> 
        <br> 
        <input type="text" placeholder="Thời gian sáng từng bậc" name="timeset" id="timeset" >   
        <br> 
        <br> 
        <input type="text" placeholder="Thời gian tắt led" name="timeoff" id="timeoff" >   
        <br>
        <br>          
        <button type="submit">Cấu Hình</button>
      </form>
    </div>
    <div class="copyright">&copy; congchuc289@gmail.com</div>  
  </div> 
  <script type="text/javascript">
    function validateform() {
      if (document.getElementById("mode_act").value == '') {
          alert("Nhập chế độ, sau đó tiến hành cài đặt");
          return false;
        }
      if(document.getElementById("mode_act").value <= 0 ||  document.getElementById("mode_act").value > 7){
          alert("Vui lòng chọn chế độ từ 1 - 7");
          return false;
        }
      else{
        alert("Cấu hình thành công - chờ 5s để khởi động lại");
        return true;
      }
    } 
    function UpdateData() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          var DataVDK = xhttp.responseText;
          console.log("Dữ liệu VDK:" + DataVDK);   
          var DataJson = JSON.parse(DataVDK); 
          document.getElementById("mode").value = DataJson.mode_act;
          document.getElementById("number_step").value = DataJson.number_step;
          document.getElementById("timeset").value = DataJson.timeset;
          document.getElementById("timeoff").value = DataJson.timeoff;
        }
      }
      xhttp.open('GET', '/sendweb', true);
      xhttp.send(); 
      setTimeout(function () { UpdateData() }, 1000); 
    }       
  </script>
</body>
</html>
)=====";
