class HomePage {
    constructor() {
        this.pdiv = null;
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
            '</div>';

        this.pdiv.innerHTML = s;
    }
}
