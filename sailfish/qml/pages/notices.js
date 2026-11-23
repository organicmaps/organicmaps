.import Sailfish.Silica 1.0 as Silica

// Short messages in the middle of the screen.
function show(text) {
    Silica.Notices.show(text, Silica.Notice.Short, Silica.Notice.Center)
}

function showLong(text) {
    Silica.Notices.show(text, Silica.Notice.Long, Silica.Notice.Center)
}
