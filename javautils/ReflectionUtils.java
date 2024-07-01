import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

public class ReflectionUtils {
    public Object instance;
    public Class<?> instanceClass;
    public ConcurrentMap<String, Field> fieldCache;
    public ConcurrentMap<String, Method> methodCache;
    public ConcurrentMap<String, Method> superMethodCache;

    public ReflectionUtils(Object instance) {
        this.instance = instance;
        this.instanceClass = instance.getClass();
        this.fieldCache = new ConcurrentHashMap<>();
        this.methodCache = new ConcurrentHashMap<>();
        this.superMethodCache = new ConcurrentHashMap<>();
    }

    public Field getField(String fieldName) {
        return fieldCache.computeIfAbsent(fieldName, name -> {
            try {
                Field field = instanceClass.getDeclaredField(name);
                field.setAccessible(true);
                return field;
            } catch (NoSuchFieldException e) {
                return null;
            }
        });
    }

    public Method getMethod(String methodName, Class<?>... parameterTypes) {
        String methodKey = getMethodKey(methodName, parameterTypes);
        return methodCache.computeIfAbsent(methodKey, key -> {
            try {
                Method method = instanceClass.getDeclaredMethod(methodName, parameterTypes);
                method.setAccessible(true);
                return method;
            } catch (NoSuchMethodException e) {
                return null;
            }
        });
    }

    public Method getSuperMethod(String methodName, Class<?>... parameterTypes) {
        String methodKey = getMethodKey(methodName, parameterTypes);
        return superMethodCache.computeIfAbsent(methodKey, key -> {
            try {
                Method method = instanceClass.getSuperclass().getDeclaredMethod(methodName, parameterTypes);
                method.setAccessible(true);
                return method;
            } catch (NoSuchMethodException e) {
                return null;
            }
        });
    }

    public String getMethodKey(String methodName, Class<?>... parameterTypes) {
        StringBuilder key = new StringBuilder(methodName);
        for (Class<?> paramType : parameterTypes) {
            key.append("#").append(paramType.getName());
        }
        return key.toString();
    }

    public Object getValue(String fieldName) {
        try {
            Field field = getField(fieldName);
            return field.get(instance);
        } catch (Exception e) {
            return null;
        }
    }

    public void setValue(String fieldName, Object value) {
        try {
            Field field = getField(fieldName);
            field.set(instance, value);
        } catch (Exception e) {
            return;
        }
    }

    public Object invokeMethod(String methodName, Object... args) {
        try {
            Class<?>[] argTypes = Arrays.stream(args)
                    .map(arg -> {
                        if (arg instanceof Integer) return int.class;
                        if (arg instanceof Float) return float.class;
                        if (arg instanceof Double) return double.class;
                        if (arg instanceof Boolean) return boolean.class;
                        if (arg instanceof Long) return long.class;
                        if (arg instanceof Character) return char.class;
                        if (arg instanceof Byte) return byte.class;
                        if (arg instanceof Short) return short.class;
                        return arg.getClass();
                    })
                    .toArray(Class<?>[]::new);
            Method method = getMethod(methodName, argTypes);
            return method.invoke(instance, args);
        } catch (Exception e) {
            return null;
        }
    }

    public Object invokeSuperMethod(String methodName, Object... args) {
        try {
            Class<?>[] argTypes = Arrays.stream(args)
                    .map(arg -> {
                        if (arg instanceof Integer) return int.class;
                        if (arg instanceof Float) return float.class;
                        if (arg instanceof Double) return double.class;
                        if (arg instanceof Boolean) return boolean.class;
                        if (arg instanceof Long) return long.class;
                        if (arg instanceof Character) return char.class;
                        if (arg instanceof Byte) return byte.class;
                        if (arg instanceof Short) return short.class;
                        return arg.getClass();
                    })
                    .toArray(Class<?>[]::new);
            Method superMethod = getSuperMethod(methodName, argTypes);
            return superMethod.invoke(instance, args);
        } catch (Exception e) {
            return null;
        }
    }
}