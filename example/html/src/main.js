var boardInfo  = new BoardInfo();
var statusInfo = new StatusInfo();

class App {
    constructor(logger) {
        //
    }

    InfoFromString(jStr) {
        boardInfo.setFromString(jStr);
        this.toPage();
        systemPage.updateInfo();
    }

    toPage() {
        document.title = boardInfo.get('title');

        let a = document.getElementById('title');
        if (a !== null)
            a.innerHTML = boardInfo.get('title');

        a = document.getElementById('tagline');
        if (a !== null) {
            a.innerHTML = boardInfo.get('tagline');
            a.hidden = (a.innerHTML.length > 0);
        }
    }

    StatusFromString(jStr) {
        statusInfo.setFromString(jStr);
        homePage.updateInfo();
    }

    Initialize() {
        logger.set_parent_div(document.getElementById('LOG'));

        let appel = document.getElementById('APP');
        homePage.set_parent_div(appel);
        configPage.set_parent_div(appel);
        systemPage.set_parent_div(appel);

        systemPage.set_info_object(boardInfo);
        homePage.set_status_object(statusInfo);

        this.GetInfo();
        this.GetConfig();
        this.GetStatus();

        this.statusTimer = setInterval(function() { app.GetStatus(); }, 2000);

        this.HashHandler();
        window.addEventListener('hashchange', this.HashHandler, false);
    }

    SaveConfig() {
        let xhr = new XMLHttpRequest();
        xhr.onload = function() {
            if (xhr.readyState === xhr.DONE) {
                if (xhr.status === 200) {
                    logger.info(xhr.responseText);
                }
                else {
                    logger.error(xhr.status + " " + xhr.responseText);
                }
            }
        };
        xhr.onerror = function() { logger.error("Send error"); };
        xhr.onabort = function() { logger.warning("Send canceled"); };

        xhr.open("POST", "/config.json", true);
        xhr.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
        xhr.send(configuration.fromPagetoString());
    }

    GetConfig() {
        let xhr = new XMLHttpRequest();
        xhr.onload = function() {
            if (xhr.readyState === xhr.DONE) {
                if (xhr.status === 200) {
                    configuration.fromString(xhr.responseText);
                }
            }
        };
        xhr.open("GET", "/config.json", true);
        xhr.send();
    }

    GetInfo() {
        let xhr = new XMLHttpRequest();
        xhr.onload = function() {
            if (xhr.readyState === xhr.DONE) {
                if (xhr.status === 200) {
                    app.InfoFromString(xhr.responseText);
                }
            }
        };
        xhr.open("GET", "/info.json", true);
        xhr.send();
    }

    GetStatus() {
        let xhr = new XMLHttpRequest();
        xhr.onload = function() {
            if (xhr.readyState === xhr.DONE) {
                if (xhr.status === 200) {
                    app.StatusFromString(xhr.responseText);
                }
            }
        };
        xhr.open("GET", "/status.json", true);
        xhr.send();
    }

    SendCmd(cmdID, data) {
        let xhr = new XMLHttpRequest();
        xhr.onload = function() {
            if (xhr.readyState === xhr.DONE) {
                if (xhr.status === 200) {
                    logger.info(xhr.responseText);
                }
                else {
                    logger.error(xhr.status + " " + xhr.responseText);
                }
            }
        };
        xhr.onerror = function() { logger.error("Send error"); };
        xhr.onabort = function() { logger.warning("Send canceled"); };

        let str = JSON.stringify({ "cmd": cmdID, "data": data });

        xhr.open("POST", "/cmd.json", true);
        xhr.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
        xhr.send(str);
    }

    SendCommand1() {
        let str = document.getElementById('userData').value;
        let val = parseInt(str, 16);
        this.SendCmd(1, val);
    }
    SendCommand2() {
        let str = document.getElementById('userData').value;
        let val = parseInt(str, 16);
        this.SendCmd(2, val);
    }
    SendCommand3() {
        let str = document.getElementById('userData').value;
        let val = parseInt(str, 16);
        this.SendCmd(3, val);
    }
    ResetBoard() {
        this.SendCmd(0xFE, 0);
    }

    UpFW() {
        let f = systemPage.GetSelectedFile();
        if (f == null) return;

        let xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (xhr.readyState === xhr.DONE) {
                if (xhr.status === 200) {
                    logger.info(xhr.responseText);
                }
                else {
                    logger.error(xhr.status + " " + xhr.responseText);
                }
                systemPage.EndUpload();
            }
        };

        xhr.upload.addEventListener("progress", function(ev) {
            if (ev.lengthComputable) {
                let percent = 100 * ev.loaded / ev.total | 0;
                systemPage.SetUploadProgress(percent);
            }
        });

        systemPage.BeginUpload();

        xhr.open("POST", "/update", true);
        xhr.send(f);
    }

    TogglePass(b, id) {
        let e = document.querySelector('#' + id);
        if (e == null) return;

        if (e.getAttribute('type') === 'text' ) {
            e.setAttribute('type', 'password');
            b.innerText = 'O';
        }
        else {
            e.setAttribute('type', 'text');
            b.innerText = 'X';
        }
    }

    HashHandler() {
        if (location.hash === "#home") {
            homePage.render();
            return;
        }

        if (location.hash === "#config") {
            configPage.render();
            return;
        }

        if (location.hash === "#system") {
            systemPage.render();
            return;
        }

        // treat everything else like #home
        homePage.render();
    }
}

var logger        = new Logger();
var homePage      = new HomePage();
var configuration = new Configuration();
var configPage    = new ConfigPage();

var app = new App();
app.Initialize();
