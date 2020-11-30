class HomePage {
    constructor() {
        this.pdiv = null;
    }

    set_parent_div(parentDiv) {
        this.pdiv = parentDiv;
    }
    render() {
        if (!this.pdiv) return;

        let str = '';

        str = str + '<h3>Send some commands and data</h3>';
        str = str + '<input id="userData" type="text" value="" />';
        str = str + '<button class="mrgLeft" onclick="app.SendCommand1()">Command 1</button>';
        str = str + '<button class="mrgLeft" onclick="app.SendCommand2()">Command 2</button>';
        str = str + '<button class="mrgLeft" onclick="app.SendCommand3()">Command 3</button>';

        this.pdiv.innerHTML = str;
    }
}
