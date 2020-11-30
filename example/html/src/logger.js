class Logger {
    constructor() {
        this.messages = [];
        this.maxMsg = 5;
        this.pdiv = undefined;
    }

    _addMessage(msg) {
        this.messages.unshift(msg);
        if (this.messages.length > this.maxMsg) {
            this.messages.pop();
        }
        this.render();
    }

    info(str) {
        let message = { "type": "info", "date": Date.now(), "msg": str };
        this._addMessage(message);
    }
    warning(str) {
        let message = { "type": "warning", "date": Date.now(), "msg": str };
        this._addMessage(message);
    }
    error(str) {
        let message = { "type": "error", "date": Date.now(), "msg": str };
        this._addMessage(message);
    }

    set_parent_div(parentDiv) {
        this.pdiv = parentDiv;
    }
    render() {
        if (this.pdiv === undefined) return;

        let xstr = '';
        for (let i = 0; i < this.messages.length; ++i){
            let message = this.messages[i];
            let d = new Date();
            d.setTime(message.date);

            let str = '<p class="log-' + message.type + '">' +
                d.toISOString() +
                ' ' +
                message.msg +
                '</p>';

            xstr = xstr + str;
        }
        this.pdiv.innerHTML = xstr;
    }
}
