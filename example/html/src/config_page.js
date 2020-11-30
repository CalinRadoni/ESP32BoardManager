class ConfigPage {
    constructor() {
        this.pdiv = null;
    }

    set_parent_div(parentDiv) {
        this.pdiv = parentDiv;
    }
    render() {
        if (!this.pdiv) return;

        let s = '<div class="config">' +
            '<div class="cfgrow"><div class="cfgcell"><h3>Configuration</h3></div></div>' +
            '<div class="cfgrow">' +
                this.addTextInput('Name', 'pname', '') +
                this.addPasswordInput('Pass', 'ppass', '') +
            '</div>' +
            '<div class="cfgrow">' +
                this.addTextInput('SSID', 'pap1s', '') +
                this.addPasswordInput('Password', 'pap1p', '') +
            '</div>' +
            '<div class="cfgrow">' +
                this.addTextInput('Backup SSID', 'pap2s', '') +
                this.addPasswordInput('Backup password', 'pap2p', '') +
            '</div>' +
            '<div class="cfgrow"><div class="cfgcell"><p class="cfgI" id="pversion"></p></div></div>' +
            '<div class="cfgrow">' +
            '<div class="cfgcell aright">' +
                '<button class="mrgLeft" onclick="app.GetConfig()">Reload</button>' +
                '<button class="mrgLeft" onclick="app.SaveConfig()">Save</button>' +
            '</div></div>' +
            '</div>';

        this.pdiv.innerHTML = s;

        configuration.toPage();
    }

    addTextInput(label, id, value) {
        let s = '<div class="cfgcell">' +
                '<label class="cfgL" for="' + id + '">' + label + '</label>' +
                '<div class="srow">' +
                '<input class="scell" type="text" id="' + id + '" value="' + value + '">' +
                '</div></div>';
        return s;
    }

    addPasswordInput(label, id, value) {
        let s = '<div class="cfgcell">' +
                '<label class="cfgL" for="' + id + '">' + label + '</label>' +
                '<div class="srow">' +
                '<input class="scell" type="password" id="' + id + '" value="' + value + '">' +
                '<button class="shb" onclick="app.TogglePass(this, &quot;' + id + '&quot;)">show</button>' +
                '</div></div>';
        return s;
    }
}
