#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define MAX_VERTEX_COUNT 30

#define VERTEX_RADIUS 12

Font font;

typedef struct
{
  size_t* items;
  size_t start;
  size_t end;
  size_t size;
  size_t capacity;
} Queue;

void
enqueue(Queue* q, size_t elem)
{
  if (q->end == q->capacity) {
    q->capacity = 2 * q->capacity + 1;
    q->items = realloc(q->items, q->capacity * sizeof(q->items[0]));
  }
  q->items[q->end++] = elem;
}

size_t
dequeue(Queue* q)
{
  return q->items[q->start++];
}

typedef struct
{
  size_t vertex_count;
  size_t adj[MAX_VERTEX_COUNT][MAX_VERTEX_COUNT];
  size_t adj_count[MAX_VERTEX_COUNT];
  Vector2 vertex_pos[MAX_VERTEX_COUNT];
  Color vertex_color[MAX_VERTEX_COUNT];
} Graph;

void
draw_graph(Graph* g)
{

  for (size_t i = 0; i < g->vertex_count; ++i) {
    for (size_t j = 0; j < g->adj_count[i]; ++j) {
      Vector2 c1 = g->vertex_pos[i];
      Vector2 c2 = g->vertex_pos[g->adj[i][j]];
      DrawLine(c1.x, c1.y, c2.x, c2.y, BLACK);
    }
  }

  for (size_t i = 0; i < g->vertex_count; ++i) {
    Vector2 center = g->vertex_pos[i];
    DrawCircle(center.x, center.y, VERTEX_RADIUS, BLACK);
    DrawCircle(center.x, center.y, VERTEX_RADIUS - 2, g->vertex_color[i]);
    char buffer[20] = { 0 };
    sprintf(buffer, "%d", i);
    Vector2 text_size = MeasureTextEx(font, buffer, VERTEX_RADIUS * 1.2f, 0.0f);
    DrawText(buffer,
             center.x - text_size.x * 0.5f,
             center.y - text_size.y * 0.5f,
             VERTEX_RADIUS * 1.2f,
             BLACK);
  }
}

typedef enum
{
  COLOR_VERTEX,
  CHECK_VERTEX,
  ADD_VERTEX,
  ADD_EDGE,
} Step_Kind;

typedef struct
{
  size_t vertex_idx;
  Color from;
  Color to;
} Color_Vertex;

typedef struct
{
  size_t vertex_idx;
} Check_Vertex;

typedef struct
{
  size_t vertex_idx;
} Add_Vertex;

typedef struct
{
  size_t vertex1;
  size_t vertex2;
} Add_Edge;

typedef union
{
  Color_Vertex color_vertex;
  Check_Vertex check_vertex;
  Add_Vertex add_vertex;
  Add_Edge add_edge;
} Step_As;

typedef struct
{
  Step_Kind kind;
  Step_As as;
} Step;

typedef struct
{
  Step* items;
  size_t size;
  size_t capacity;
} Animation;

void
add_step_color_vertex(Animation* a, size_t v, Color from, Color to)
{
  Step step = { 0 };
  step.kind = COLOR_VERTEX;
  step.as.color_vertex =
    (Color_Vertex){ .vertex_idx = v, .from = from, .to = to };
  if (a->size == a->capacity) {
    a->capacity = 2 * a->capacity + 1;
    a->items = realloc(a->items, a->capacity * sizeof(a->items[0]));
  }
  a->items[a->size++] = step;
}

// assumes graph is painted WHITE
void
visit_bfs(Graph g, size_t s, Animation* a)
{
  Queue q = { 0 };
  enqueue(&q, s);
  while (q.end != q.start) {
    size_t v = dequeue(&q);
    g.vertex_color[v] = GRAY;
    add_step_color_vertex(a, v, WHITE, GRAY);
    for (size_t i = 0; i < g.adj_count[v]; ++i) {
      size_t u = g.adj[v][i];
      if (ColorToInt(g.vertex_color[u]) == ColorToInt(WHITE)) {
        enqueue(&q, u);
        g.vertex_color[u] = GRAY;
        add_step_color_vertex(a, u, WHITE, GRAY);
      }
    }
    g.vertex_color[v] = RED;
    add_step_color_vertex(a, v, GRAY, RED);
  }
}

int
main(void)
{
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "graphz");
  font = GetFontDefault();

  Graph g = { .vertex_count = 2,
              .adj = { { 1 }, { 0 } },
              .adj_count = { 1, 1 },
              .vertex_pos = { { .x = 50, .y = 50 }, { .x = 100, .y = 100 } },
              .vertex_color = { WHITE, WHITE } };
  Animation a = { 0 };
  size_t animation_idx = 0;

  visit_bfs(g, 0, &a);

  SetTargetFPS(60);
  while (!WindowShouldClose()) {

    if (IsKeyPressed(KEY_N)) {
      Step step = a.items[animation_idx++];
      switch (step.kind) {
        case COLOR_VERTEX: {
          Color_Vertex color_operation = step.as.color_vertex;
          g.vertex_color[color_operation.vertex_idx] = color_operation.to;
        } break;

        default:
          break;
      }
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    draw_graph(&g);
    EndDrawing();
  }

  return 0;
}
