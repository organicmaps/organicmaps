// Source - https://stackoverflow.com/a/65082990
// Posted by Leo
// Retrieved 2026-06-30, License - CC BY-SA 4.0

import QtQuick 2.0
import Map

Item {
    id: root
    required property DrawWidget map

    width: 400
    height: 300
    Canvas {
        id: cloud
        anchors.fill: parent

        function cloudPath(ctx) {
            var x = 100;
            var y = 170;
            ctx.beginPath();
            ctx.arc(x, y, 60, Math.PI * 0.5, Math.PI * 1.5);
            ctx.arc(x + 70, y - 60, 70, Math.PI * 1, Math.PI * 1.85);
            ctx.arc(x + 152, y - 45, 50, Math.PI * 1.37, Math.PI * 1.91);
            ctx.arc(x + 200, y, 60, Math.PI * 1.5, Math.PI * 0.5);
            ctx.moveTo(x + 200, y + 60);
            ctx.lineTo(x, y + 60);
        }

        onPaint: {
            var ctx = getContext("2d");
            cloudPath(ctx);

            ctx.strokeStyle = "#797874";
            ctx.stroke();
            ctx.fillStyle = "#8ED6FF";
            ctx.fill();
        }
    }

    // No MouseArea - move with left button and right button press events are propagated
    MouseArea {
        // with MouseArea, right button is only propagated
        anchors.fill: parent  // Fills the circular area
        propagateComposedEvents: true

        function hit(mouse) {
            var ctx = cloud.getContext("2d");
            cloud.cloudPath(ctx);
            return ctx.isPointInPath(mouse.x, mouse.y);
        }

    //     // onPressed: {
    //     //     console.log("Cloud pressed");
    //     // }
    //     // onReleased: {
    //     //     console.log("Cloud released");
    //     // }
    //     // onClicked: {
    //     //     console.log("Cloud clicked");
    //     // }

        onPressed: mouse => {
            const inside = hit(mouse);
    //         mouse.accepted = inside;
            // mouse.accepted = false;     // move and right button are propagated again
            if (inside)
                console.log("Cloud pressed");
        }
        onReleased: mouse => {
            const inside = hit(mouse);
    //     //     mouse.accepted = inside;
            // mouse.accepted = false;
            if (inside)
                console.log("Cloud released");
        }
        onClicked: mouse => {
            // mouse.accepted = false;
            if (hit(mouse))
                console.log("Cloud clicked");
            else
                // map.qmlClicked1(mouse.x, mouse.y);
                root.map.qmlClicked(mouse.x, mouse.y);
        }
    }
}
