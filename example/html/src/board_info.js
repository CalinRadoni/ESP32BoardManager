const boardInfoNames = ['title', 'tagline', 'appName', 'appVersion', 'link', 'compileTime', 'idfVersion', 'elfSHA256', 'hwInfo'];
const keyPrefix = 'data_';
class BoardInfo {
    constructor() {
        this.initialize();
    }

    initialize() {
        for (let i = 0; i < boardInfoNames.length; ++i) {
            this[keyPrefix + boardInfoNames[i]] = '';
        }
    }

    setFromString(jStr) {
        this.initialize();
        let cfg = JSON.parse(jStr);

        for (const [key, value] of Object.entries(cfg)) {
            if (this.hasOwnProperty(keyPrefix + key)) {
                this[keyPrefix + key] = value;
            }
        }
    }

    get(key) {
        if (this.hasOwnProperty(keyPrefix + key))
            return this[keyPrefix + key];

        return '';
    }
}
