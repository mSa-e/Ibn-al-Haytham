#ifndef PORTAL_STUDENTDATA_H
#define PORTAL_STUDENTDATA_H

struct TermCourse   { const char *code, *title; int credits; const char *grade; };
struct Term         { const char *name; const struct TermCourse *courses; int count; };
struct CurrentCourse{ const char *code, *title, *instructor; int credits, midterm, midterm_max, attendance; };
struct ScheduleSlot { const char *day, *time, *code, *kind, *room; };
struct Deadline     { const char *date, *code, *title; };
struct FeeItem      { const char *item, *due, *status; int amount; };
struct Announcement { const char *date, *title, *text; };

struct Student {
    const char *name, *id, *faculty, *program, *advisor, *email, *current_term;
    int admission_year, level;
    const struct Term *terms;               int term_count;
    const struct CurrentCourse *courses;    int course_count;
    const struct ScheduleSlot *schedule;    int schedule_count;
    const struct Deadline *deadlines;       int deadline_count;
    const struct FeeItem *fees;             int fee_count;
};

/* Login: the name (case-insensitive, surrounding spaces ignored) and the password, which is the
 * student ID, must both match. Returns the student, or NULL. */
const struct Student *student_authenticate(const char *name, const char *password);

const struct Announcement *portal_announcements(int *count);
double grade_points(const char *letter_grade);      /* 4.0 scale; unknown grades count as 0 */
#endif
