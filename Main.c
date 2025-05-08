#include <Misra/Parsers/JSON.h>
#include <Misra/Std.h>
#include <Misra/Types.h>

typedef Vec(Str) Strs;

int main(int argc, char** argv) {
    LogInit(false);

    Str json = StrInitFromZstr(
        "{   \"name\"  :    \"misra\", \"data\":{\"x_axis_val\":-22.24485,\"gname\":\"a random "
        "graph\",\"y_axis_val\":133.455234} ,\"ref\":40, \"strs\":[\"x\", \"ah _ ha\", \"lessa do something\"]}"
    );
    StrIter si = StrIterFromStr(&json);

    struct {
        struct {
            float x;
            float y;
            Str   n;
        } data;
        Str  name;
        int  ref;
        Strs strs;
    } obj = {0};

    obj.strs = (Strs)VecInit();

    JR_OBJ(si, {
        JR_INT_KV(si, "ref", obj.ref);
        JR_OBJ_KV(si, "data", {
            JR_FLT_KV(si, "y_axis_val", obj.data.y);
            JR_FLT_KV(si, "x_axis_val", obj.data.x);
            JR_STR_KV(si, "gname", obj.data.n);
        });
        JR_STR_KV(si, "name", obj.name);
        JR_ARR_KV(si, "strs", {
            Str tmp_s;
            JR_STR(si, tmp_s);
            VecPushBack(&obj.strs, tmp_s);
        });
    });

    printf("Name : %s\n", obj.name.data);
    printf("Ref : %d\n", obj.ref);
    printf("X : %f\n", obj.data.x);
    printf("X : %f\n", obj.data.y);
    printf("N : %s\n", obj.data.n.data);
    printf("strs : [");
    VecForeach(&obj.strs, str, { printf("%s, ", str.data); });
    printf("]\n");

    StrClear(&json);

    JW_OBJ(json, {
        JW_INT_KV(json, "ref", obj.ref);
        JW_OBJ_KV(json, "data", {
            JW_FLT_KV(json, "y_axis_val", obj.data.y);
            JW_FLT_KV(json, "x_axis_val", obj.data.x);
            JW_STR_KV(json, "gname", obj.data.n);
        });
        JW_STR_KV(json, "name", obj.name);
        JW_ARR_KV(json, "strs", obj.strs, s, { JW_STR(json, s); });
    });

    // {"ref":40,"data":{"y_axis_val":133.455231,"x_axis_val":-22.244850,"gname":"a random graph"},"name":"misra","strs":["x","ah _ ha","lessa do something"]}
    printf("%s\n", json.data);

    LogDeinit();
    return 0;
}
