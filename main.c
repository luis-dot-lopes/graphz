#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define MAX_VERTEX_COUNT 30

#define VERTEX_RADIUS 12
#define ARROW_LENGTH 8

Font font;

typedef struct
{
  size_t* items;
  size_t start;
  size_t end;
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

void
free_queue(Queue* q)
{
  q->start = q->end = q->capacity = 0;
  free(q->items);
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
draw_graph(Graph* g,
           size_t highlight_idx,
           size_t clicked_vertex1,
           size_t clicked_vertex2)
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
    Color border_color = BLACK;
    if (i == highlight_idx) {
      border_color = BLUE;
    } else if (i == clicked_vertex1 || i == clicked_vertex2) {
      border_color = GREEN;
    }
    DrawCircle(center.x, center.y, VERTEX_RADIUS, border_color);
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

void
draw_directed_graph(Graph* g,
                    size_t highlight_idx,
                    size_t clicked_vertex1,
                    size_t clicked_vertex2)
{
  for (size_t i = 0; i < g->vertex_count; ++i) {
    for (size_t j = 0; j < g->adj_count[i]; ++j) {
      Vector2 c1 = g->vertex_pos[i];
      Vector2 c2 = g->vertex_pos[g->adj[i][j]];
      Vector2 unit_line = Vector2Normalize(Vector2Subtract(c1, c2));
      Vector2 touch_point =
        Vector2Add(c2, Vector2Scale(unit_line, VERTEX_RADIUS));
      Vector2 arrow1 =
        Vector2Scale(Vector2Rotate(unit_line, DEG2RAD * 30.0f), ARROW_LENGTH);
      Vector2 arrow2 =
        Vector2Scale(Vector2Rotate(unit_line, DEG2RAD * -30.0f), ARROW_LENGTH);
      DrawLine(c1.x, c1.y, c2.x, c2.y, BLACK);
      DrawLineV(touch_point, Vector2Add(touch_point, arrow1), BLACK);
      DrawLineV(touch_point, Vector2Add(touch_point, arrow2), BLACK);
    }
  }

  for (size_t i = 0; i < g->vertex_count; ++i) {
    Vector2 center = g->vertex_pos[i];
    Color border_color = BLACK;
    if (i == highlight_idx) {
      border_color = BLUE;
    } else if (i == clicked_vertex1 || i == clicked_vertex2) {
      border_color = GREEN;
    }
    DrawCircle(center.x, center.y, VERTEX_RADIUS, border_color);
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
  step.as.color_vertex = (Color_Vertex){
    .vertex_idx = v,
    .from = from,
    .to = to,
  };
  if (a->size == a->capacity) {
    a->capacity = 2 * a->capacity + 1;
    a->items = realloc(a->items, a->capacity * sizeof(a->items[0]));
  }
  a->items[a->size++] = step;
}

void
add_step_check_vertex(Animation* a, size_t v)
{
  Step step = { 0 };
  step.kind = CHECK_VERTEX;
  step.as.check_vertex = (Check_Vertex){
    .vertex_idx = v,
  };
  if (a->size == a->capacity) {
    a->capacity = 2 * a->capacity + 1;
    a->items = realloc(a->items, a->capacity * sizeof(a->items[0]));
  }
  a->items[a->size++] = step;
}

void
add_step_add_edge(Animation* a, size_t v, size_t u)
{
  Step step = { 0 };
  step.kind = ADD_EDGE;
  step.as.add_edge = (Add_Edge){
    .vertex1 = v,
    .vertex2 = u,
  };
  if (a->size == a->capacity) {
    a->capacity = 2 * a->capacity + 1;
    a->items = realloc(a->items, a->capacity * sizeof(a->items[0]));
  }
  a->items[a->size++] = step;
}

void
free_animation(Animation* a)
{
  a->capacity = a->size = 0;
  free(a->items);
  a->items = NULL;
}

void
paint_graph(Graph* g, Color color)
{
  for (size_t i = 0; i < g->vertex_count; ++i) {
    g->vertex_color[i] = color;
  }
}

// assumes graph is painted WHITE
void
visit_bfs(Graph g, size_t s, Animation* a)
{
  Queue q = { 0 };
  enqueue(&q, s);
  while (q.end != q.start) {
    size_t v = dequeue(&q);
    add_step_check_vertex(a, v);
    g.vertex_color[v] = GRAY;
    add_step_color_vertex(a, v, WHITE, GRAY);
    for (size_t i = 0; i < g.adj_count[v]; ++i) {
      size_t u = g.adj[v][i];
      add_step_check_vertex(a, u);
      if (ColorToInt(g.vertex_color[u]) == ColorToInt(WHITE)) {
        enqueue(&q, u);
        g.vertex_color[u] = GRAY;
        add_step_color_vertex(a, u, WHITE, GRAY);
      }
    }
    g.vertex_color[v] = RED;
    add_step_color_vertex(a, v, GRAY, RED);
  }
  free_queue(&q);
}

void
visit_dfs(Graph g, size_t s, Animation* a)
{
  size_t stack[MAX_VERTEX_COUNT];
  size_t stack_size = 0;
  stack[stack_size++] = s;
  while (stack_size != 0) {
    size_t v = stack[stack_size - 1];
    add_step_check_vertex(a, v);
    g.vertex_color[v] = GRAY;
    add_step_color_vertex(a, v, WHITE, GRAY);
    for (size_t i = 0; i < g.adj_count[v]; ++i) {
      size_t u = g.adj[v][i];
      add_step_check_vertex(a, u);
      if (ColorToInt(g.vertex_color[u]) == ColorToInt(WHITE)) {
        stack[stack_size++] = u;
        goto next;
      }
    }
    --stack_size;
    g.vertex_color[v] = RED;
    add_step_color_vertex(a, v, GRAY, RED);
  next:;
  }
}

// Uses a BLACK vertex to indicate failure
void
paint_bipartite(Graph g, size_t s, Animation* a)
{
  Queue q = { 0 };
  enqueue(&q, s);
  g.vertex_color[s] = BLUE;
  add_step_color_vertex(a, s, WHITE, BLUE);
  while (q.end != q.start) {
    size_t v = dequeue(&q);
    add_step_check_vertex(a, v);
    for (size_t i = 0; i < g.adj_count[v]; ++i) {
      size_t u = g.adj[v][i];
      add_step_check_vertex(a, u);
      if (ColorToInt(g.vertex_color[u]) == ColorToInt(WHITE)) {
        if (ColorToInt(g.vertex_color[v]) == ColorToInt(BLUE)) {
          g.vertex_color[u] = RED;
          add_step_color_vertex(a, u, WHITE, RED);
        } else if (ColorToInt(g.vertex_color[v]) == ColorToInt(RED)) {
          g.vertex_color[u] = BLUE;
          add_step_color_vertex(a, u, WHITE, BLUE);
        }
        enqueue(&q, u);
      } else if (ColorToInt(g.vertex_color[v]) ==
                 ColorToInt(g.vertex_color[u])) {
        add_step_color_vertex(a, u, g.vertex_color[u], BLACK);
        free_queue(&q);
        return;
      }
    }
  }
  free_queue(&q);
}

void
make_graph_connected(Graph g, Animation* a)
{
  Queue q = { 0 };
  size_t least_white = 0;
  size_t connect1 = g.vertex_count, connect2 = g.vertex_count;
  while (least_white < g.vertex_count) {
    if (ColorToInt(g.vertex_color[least_white]) == ColorToInt(WHITE)) {
      connect1 = connect2;
      connect2 = least_white;
      if (connect1 < g.vertex_count && connect2 < g.vertex_count) {
        add_step_add_edge(a, connect1, connect2);
        add_step_add_edge(a, connect2, connect1);
      }
      enqueue(&q, least_white);
      while (q.end != q.start) {
        size_t v = dequeue(&q);
        add_step_check_vertex(a, v);
        g.vertex_color[v] = GRAY;
        add_step_color_vertex(a, v, WHITE, GRAY);
        for (size_t i = 0; i < g.adj_count[v]; ++i) {
          size_t u = g.adj[v][i];
          add_step_check_vertex(a, u);
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
    ++least_white;
  }
  free_queue(&q);
}

int
main(void)
{
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "graphz");
  font = GetFontDefault();

  Graph g = { .vertex_count = 4,
              .adj = { { 1, 2, 3 }, { 0, 2 }, { 0, 1 }, { 0 } },
              .adj_count = { 3, 2, 2, 1 },
              .vertex_pos = { { .x = 50, .y = 50 },
                              { .x = 50, .y = 100 },
                              { .x = 100, .y = 100 },
                              { .x = 100, .y = 50 } },
              .vertex_color = { WHITE, WHITE, WHITE, WHITE } };
  Animation a = { 0 };
  size_t animation_idx = 0;
  size_t highlight_idx = MAX_VERTEX_COUNT + 1;
  size_t selected_vertex = MAX_VERTEX_COUNT + 1;
  size_t clicked_vertex1 = MAX_VERTEX_COUNT + 1;
  size_t clicked_vertex2 = MAX_VERTEX_COUNT + 1;

  make_graph_connected(g, &a);

  SetTargetFPS(60);
  while (!WindowShouldClose()) {

    if (IsKeyPressed(KEY_N) && animation_idx < a.size) {
      highlight_idx = g.vertex_count;
      Step step = a.items[animation_idx++];
      switch (step.kind) {
        case COLOR_VERTEX: {
          Color_Vertex color_operation = step.as.color_vertex;
          g.vertex_color[color_operation.vertex_idx] = color_operation.to;
        } break;

        case CHECK_VERTEX: {
          Check_Vertex check_operation = step.as.check_vertex;
          highlight_idx = check_operation.vertex_idx;
        } break;

        case ADD_EDGE: {
          Add_Edge add_edge_operation = step.as.add_edge;
          size_t v = add_edge_operation.vertex1, u = add_edge_operation.vertex2;
          if (g.adj_count[v] < MAX_VERTEX_COUNT) {
            g.adj[v][g.adj_count[v]++] = u;
          } else {
            TraceLog(LOG_ERROR, "Too many edges. Animation step skipped");
          }
        } break;

        default:
          break;
      }
    } else if (IsKeyPressed(KEY_A) && g.vertex_count < MAX_VERTEX_COUNT) {
      g.vertex_pos[g.vertex_count] =
        (Vector2){ .x = SCREEN_WIDTH / 2, .y = SCREEN_HEIGHT / 2 };
      g.vertex_color[g.vertex_count++] = WHITE;
      clicked_vertex1 = clicked_vertex2 = selected_vertex =
        MAX_VERTEX_COUNT + 1;
    } else if (IsKeyPressed(KEY_R)) {
      free_animation(&a);
      animation_idx = 0;
      paint_graph(&g, WHITE);
      make_graph_connected(g, &a);
    }

    int mx = GetMouseX(), my = GetMouseY();
    if (selected_vertex < g.vertex_count) {
      g.vertex_pos[selected_vertex].x = mx;
      g.vertex_pos[selected_vertex].y = my;
    } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      for (size_t i = 0; i < g.vertex_count; ++i) {
        Vector2 v_pos = g.vertex_pos[i];
        if ((mx - v_pos.x) * (mx - v_pos.x) + (my - v_pos.y) * (my - v_pos.y) <=
            VERTEX_RADIUS * VERTEX_RADIUS) {
          selected_vertex = i;
          break;
        }
      }
    }
    if (IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
      selected_vertex = MAX_VERTEX_COUNT + 1;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      int found = 0;
      for (size_t i = 0; i < g.vertex_count; ++i) {
        Vector2 v_pos = g.vertex_pos[i];
        if ((mx - v_pos.x) * (mx - v_pos.x) + (my - v_pos.y) * (my - v_pos.y) <=
            VERTEX_RADIUS * VERTEX_RADIUS) {
          if (clicked_vertex1 < g.vertex_count)
            clicked_vertex2 = i;
          else
            clicked_vertex1 = i;
          found = 1;
          break;
        }
      }
      if (!found) {
        clicked_vertex1 = clicked_vertex2 = MAX_VERTEX_COUNT + 1;
      }
    }

    if (clicked_vertex1 < g.vertex_count && clicked_vertex2 < g.vertex_count) {
      g.adj[clicked_vertex1][g.adj_count[clicked_vertex1]++] = clicked_vertex2;
      g.adj[clicked_vertex2][g.adj_count[clicked_vertex2]++] = clicked_vertex1;
      clicked_vertex1 = clicked_vertex2 = MAX_VERTEX_COUNT + 1;
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    draw_graph(&g, highlight_idx, clicked_vertex1, clicked_vertex2);
    EndDrawing();
  }

  free_animation(&a);

  return 0;
}
