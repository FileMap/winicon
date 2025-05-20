let winicon;
try {
  winicon = require('node-gyp-build')(__dirname);
} catch (e) {
  winicon = {
    getIcon: () => { throw new Error("winicon only works on Windows."); },
    getThumbnail: () => { throw new Error("winicon only works on Windows."); }
  };
}
module.exports = winicon;