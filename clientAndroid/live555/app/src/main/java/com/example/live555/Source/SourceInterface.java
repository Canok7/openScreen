package com.example.live555.Source;

import android.util.Range;
import android.util.Size;
import android.view.Surface;

public interface SourceInterface {
      int SOURCE_SCREEN = 1;
      int SOURCE_CAMERA = 2;
     void openSource(Surface surface, SourceConfig config);
     void closeSource();
     void shiftSource(int id);

    class SourceInfo{
        public Size[] sizes;
        public Range<Integer>[] fpsRange;
    }

    class SourceConfig{
        public float fps;
        public int w;
        public int h;
    }
    SourceInfo[] getSourceInfo();
}
