package com.facebook.samples.rps;

import com.facebook.model.GraphObject;
import com.facebook.model.GraphUser;
import com.facebook.model.OpenGraphAction;
import com.facebook.model.OpenGraphObject;

public class OpenGraphUtils {

    /**
     * Used to consume GraphUser objects with an installed field
     */
    public interface GraphUserWithInstalled extends GraphUser {
        Boolean getInstalled();
    }

    /**
     * Used to create and consume Gesture open graph objects
     */
    public interface GestureGraphObject extends OpenGraphObject {
        String getTitle();
    }

    /**
     * Used to create and consume Game open graph objects.
     */
    public interface GameGraphObject extends OpenGraphObject {
        public static final String TYPE = "fb_sample_rps:game";

        GestureGraphObject getPlayerGesture();
        void setPlayerGesture(GestureGraphObject gesture);

        GestureGraphObject getOpponentGesture();
        void setOpponentGesture(GestureGraphObject gesture);

        String getResult();
        void setResult(String result);
    }

    /**
     * Used to create and consume Play open graph actions
     */
    public interface PlayAction extends OpenGraphAction {
        public static final String TYPE = "fb_sample_rps:play";
        public static final String PATH = "me/" + TYPE;
        public static final String PREVIEW_PROPERTY_NAME = "game";

        GameGraphObject getGame();

        void setGame(GameGraphObject game);
    }

    /**
     * Used to create and consume Throw open graph actions
     */
    public interface ThrowAction extends OpenGraphAction {
        public static final String TYPE = "fb_sample_rps:throw";
        public static final String PREVIEW_PROPERTY_NAME = "gesture";

        GestureGraphObject getGesture();
        void setGesture(GestureGraphObject playerGesture);

        GestureGraphObject getOpposingGesture();
        void setOpposingGesture(GestureGraphObject opposingGesture);
    }

    /**
     * Used to consume published Play open graph actions.
     */
    public interface PublishedPlayAction extends OpenGraphAction {
        PlayAction getData();

        String getType();
    }

    /**
     * Used to consume published Throw open graph actions.
     */
    public interface PublishedThrowAction extends OpenGraphAction {
        ThrowAction getData();

        String getType();
    }

    /**
     * Used to inspect the response from posting an action
     */
    public interface PostResponse extends GraphObject {
        String getId();
    }
}