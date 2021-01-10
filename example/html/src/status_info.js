const statusInfoNames = ['exampleStatusData'];
class StatusInfo {
    constructor() {
        this.keyPrefix = 'status_';
        this.initialize();
    }

    initialize() {
        for (let i = 0; i < statusInfoNames.length; ++i) {
            this[this.keyPrefix + statusInfoNames[i]] = '';
        }
    }

    setFromString(jStr) {
        this.initialize();
        let cfg = JSON.parse(jStr);

        for (const [key, value] of Object.entries(cfg)) {
            if (this.hasOwnProperty(this.keyPrefix + key)) {
                this[this.keyPrefix + key] = value;
            }
        }
    }

    get(key) {
        if (this.hasOwnProperty(this.keyPrefix + key))
            return this[this.keyPrefix + key];

        return '';
    }
}
