package com.aidoo.retrorunner;

public class RRFuncs {

    public interface Fr<R> {
        public R invoke();
    }

    public interface Fn<T> {
        public void invoke(T arg);
    }

    public interface Fnr<R, T> {
        public R invoke(T arg);
    }

    public interface Fn2<T1, T2> {
        public void invoke(T1 arg1, T2 arg2);
    }

    public interface Fnr2<R, T1, T2> {
        public R invoke(T1 arg1, T2 arg2);
    }

    public interface Fn3<T1, T2, T3> {
        public void invoke(T1 arg1, T2 arg2, T3 arg3);
    }

    public interface Fnr3<R, T1, T2, T3> {
        public R invoke(T1 arg1, T2 arg2, T3 arg3);
    }
}
