package com.aidoo.retrorunner;

public enum AspectRatio {
    FullScreen(0),
    Square(1),
    CoreProvide(2),
    Ratio4to3(3),
    Ratio16to9(4),
    AspectFill(5),
    MaxIntegerRatioScale(6);

    AspectRatio(int value) {
        rawValue = value;
    }

    public int getRawValue() {
        return rawValue;
    }

    private final int rawValue;

    public static AspectRatio valueOf(int value) {
        switch (value) {
            case 0:
                return FullScreen;
            case 1:
                return Square;
            case 2:
                return CoreProvide;
            case 3:
                return Ratio4to3;
            case 4:
                return Ratio16to9;
            case 5:
                return AspectFill;
            case 6:
                return MaxIntegerRatioScale;
            default:
                return AspectRatio.FullScreen;
        }
    }
}
