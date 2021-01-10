class HomePage {
    constructor() {
        this.pdiv = null;
        this.statusInfo = null;
    }

    set_status_object(si) {
        this.statusInfo = si;
    }
    set_parent_div(parentDiv) {
        this.pdiv = parentDiv;
    }
    render() {
        if (!this.pdiv) return;

        let s = '<h3>Send some commands and data</h3>' +
            '<div class="srow">' +
                '<label class="cfgL" for="userData">Data in hex</label>' +
                '<input id="userData" class="mLeft" type="text" value="" />' +
            '</div>' +
            '<div class="srow acenter mTop">' +
                '<button class="mTop"       onclick="app.SendCommand1()">Command 1</button>' +
                '<button class="mLeft mTop" onclick="app.SendCommand2()">Command 2</button>' +
                '<button class="mLeft mTop" onclick="app.SendCommand3()">Command 3</button>' +
            '</div>' +
            '<div class="srow mTop">' +
                '<label class="cfgL" for="userStatus">exampleStatusData</label>' +
                '<input id="userStatus" class="mLeft" type="text" value="" readonly />' +
            '</div>';

        this.pdiv.innerHTML = s;

        this.updateInfo();
    }

    updateInfo() {
        let a = document.getElementById('userStatus');
        if (a != null) {
            let val = this.statusInfo.get('exampleStatusData');
            a.value = val.toString(16);
        }
    }
}
