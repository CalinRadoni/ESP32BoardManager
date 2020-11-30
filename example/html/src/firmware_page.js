class FirmwarePage {
    constructor() {
        this.pdiv = null;
    }

    set_parent_div(parentDiv) {
        this.pdiv = parentDiv;
    }
    render() {
        if (!this.pdiv) return;

        let xstr = '';

        xstr = xstr + '<h3>Firmware</h3>';
        xstr = xstr + '<input type="file" id="fwf" accept=".bin" style="display:none" onchange="firmwarePage.SelectFW()" />';
        xstr = xstr + '<button id="fwbs" onclick="firmwarePage.ClickFW()">Select firmware file</button>';
        xstr = xstr + '<span class="mrgLeft" id="swf"></span><br/><br/>';
        xstr = xstr + '<button id="fwbu" onclick="app.UpFW()" disabled>Upload firmware</button>';
        xstr = xstr + '<span class="mrgLeft" id="swi"></span><br/><br/>';
        xstr = xstr + '<button id="fwbr" onclick="app.ResetBoard()" disabled>Reset</button>';

        this.pdiv.innerHTML = xstr;
    }

    ClickFW() {
        let fs = document.getElementById('fwf');
        if (fs == null) return;
        fs.click();
    }

    SelectFW() {
        let f = this.GetSelectedFile();
        if (f == null) return;

        let b = document.getElementById('fwbu');
        if (b != null) {
            b.disabled = false;
        }

        let d = document.getElementById('swf');
        if (d != null) {
            d.innerHTML = f.name + ", size: " + f.size + " bytes";
        }
    }

    GetSelectedFile() {
        let fs = document.getElementById('fwf');
        if (fs == null)
            return null;
        if (fs.files.length != 1)
            return null;
        return fs.files[0];
    }

    BeginUpload() {
        let b = document.getElementById('fwbs');
        if (b != null) b.disabled = true;
        b = document.getElementById('fwbu');
        if (b != null) b.disabled = true;
        b = document.getElementById('fwbr');
        if (b != null) b.disabled = true;
    }

    EndUpload() {
        let b = document.getElementById('fwbs');
        if (b != null) b.disabled = false;
        b = document.getElementById('fwbu');
        if (b != null) b.disabled = false;
        b = document.getElementById('fwbr');
        if (b != null) b.disabled = false;
    }

    SetUploadProgress(val) {
        let e = document.getElementById('swi');
        if (e == null) return;
        e.innerHTML = val + "%";
    }
}
var firmwarePage = new FirmwarePage();
