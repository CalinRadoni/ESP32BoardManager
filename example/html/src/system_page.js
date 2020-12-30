class SystemPage {
    constructor() {
        this.pdiv = null;
    }

    set_parent_div(parentDiv) {
        this.pdiv = parentDiv;
    }
    render() {
        if (!this.pdiv) return;

        let s = '<h3>Load firmware from file</h3>' +
            '<div class="srow mTop">' +
                '<input type="file" id="fwf" accept=".bin" style="display:none" onchange="systemPage.SelectFW()" />' +
                '<button id="fwbs" onclick="systemPage.ClickFW()">Select firmware file</button>' +
                '<span class="mTop dBlock" id="swf"></span>' +
            '</div>' +
            '<div class="srow mTop">' +
                '<button id="fwbu" onclick="app.UpFW()" disabled>Upload firmware</button>' +
                '<span class="mTop dBlock" id="swi"></span>' +
            '</div>' +
            '<div class="srow mTop">' +
                '<button id="fwbr" onclick="app.ResetBoard()" disabled>Reset</button>' +
            '</div>';

        this.pdiv.innerHTML = s;
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
            d.innerHTML = f.name;
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
var systemPage = new SystemPage();
