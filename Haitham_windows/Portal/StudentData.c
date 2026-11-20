#include "StudentData.h"
#include <ctype.h>
#include <string.h>

#define COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))

/* ---- Haitham's record (all values are sample data) ---------------------------------------- */

static const struct TermCourse FALL_2025[] = {
    { "CS101",  "Programming Fundamentals",   3, "A"  },
    { "MTH101", "Calculus I",                 3, "B+" },
    { "PHY101", "Physics I",                  3, "B+" },
    { "ENG101", "Technical English",          2, "A"  },
    { "HUM101", "Critical Thinking",          2, "A-" },
};
static const struct TermCourse SPRING_2026[] = {
    { "CS102",  "Object-Oriented Programming", 3, "A-" },
    { "MTH102", "Calculus II",                 3, "B"  },
    { "MTH103", "Discrete Mathematics",        3, "B+" },
    { "PHY102", "Physics II",                  3, "B"  },
    { "ENG102", "Technical Writing",           2, "A"  },
};
static const struct Term HAITHAM_TERMS[] = {
    { "Fall 2025",   FALL_2025,   COUNT(FALL_2025)   },
    { "Spring 2026", SPRING_2026, COUNT(SPRING_2026) },
};

static const struct CurrentCourse HAITHAM_COURSES[] = {
    { "CS201",  "Data Structures",             "Dr. Mona El-Sayed",   3, 24, 30, 96  },
    { "CS210",  "Computer Organization",       "Dr. Karim Fouad",     3, 21, 30, 88  },
    { "CS220",  "Systems Programming in C",    "Dr. Tarek Mansour",   3, 27, 30, 100 },
    { "MTH201", "Linear Algebra",              "Dr. Salma Abdelrahman", 3, 19, 30, 82 },
    { "STA201", "Probability and Statistics",  "Dr. Omar Hegazy",     3, 23, 30, 91  },
};

static const struct ScheduleSlot HAITHAM_SCHEDULE[] = {
    { "Sunday",    "09:00 - 10:30", "CS201",  "Lecture",  "Hall B2" },
    { "Sunday",    "11:00 - 12:30", "MTH201", "Lecture",  "Hall A1" },
    { "Monday",    "09:00 - 10:30", "CS220",  "Lecture",  "Hall B4" },
    { "Monday",    "11:00 - 13:00", "CS220",  "Lab",      "Lab C3"  },
    { "Tuesday",   "10:00 - 11:30", "STA201", "Lecture",  "Hall A3" },
    { "Tuesday",   "12:00 - 13:00", "CS201",  "Tutorial", "Room T12" },
    { "Wednesday", "09:00 - 10:30", "CS210",  "Lecture",  "Hall B2" },
    { "Wednesday", "11:00 - 12:30", "MTH201", "Tutorial", "Room T07" },
    { "Thursday",  "10:00 - 12:00", "CS201",  "Lab",      "Lab C1"  },
    { "Thursday",  "12:30 - 14:00", "CS210",  "Lab",      "Lab C5"  },
};

static const struct Deadline HAITHAM_DEADLINES[] = {
    { "24 Nov 2026", "CS201",  "Assignment 3: binary search trees" },
    { "26 Nov 2026", "MTH201", "Quiz 2: eigenvalues and eigenvectors" },
    { "30 Nov 2026", "CS220",  "Lab report 4: sockets and threads" },
    { "03 Dec 2026", "STA201", "Problem set 5: hypothesis testing" },
};

static const struct FeeItem HAITHAM_FEES[] = {
    { "Tuition - Fall 2026",      "03 Sep 2026", "Paid",     38000 },
    { "Student activities fee",   "03 Sep 2026", "Paid",      1200 },
    { "Tuition - Spring 2027",    "15 Jan 2027", "Upcoming", 38000 },
};

static const struct Student HAITHAM = {
    "Haitham", "25010000",
    "Faculty of Computer Science and Engineering", "B.Sc. Computer Science",
    "Dr. Mona El-Sayed", "haitham.25010000@horizon.edu.eg", "Fall 2026",
    2025, 2,
    HAITHAM_TERMS,     COUNT(HAITHAM_TERMS),
    HAITHAM_COURSES,   COUNT(HAITHAM_COURSES),
    HAITHAM_SCHEDULE,  COUNT(HAITHAM_SCHEDULE),
    HAITHAM_DEADLINES, COUNT(HAITHAM_DEADLINES),
    HAITHAM_FEES,      COUNT(HAITHAM_FEES),
};

static const struct Student *const STUDENTS[] = { &HAITHAM };

static const struct Announcement ANNOUNCEMENTS[] = {
    { "17 Nov 2026", "Final exam timetable published",
      "Final exams begin on 27 December 2026. Check your personal timetable and report any clash to the registrar by 1 December." },
    { "12 Nov 2026", "Spring 2027 registration",
      "Course registration opens on 14 December 2026 for Level 2 students. Meet your academic advisor before registering." },
    { "10 Nov 2026", "Library hours extended",
      "The main library is open until 22:00 on weekdays during exam preparation." },
    { "03 Nov 2026", "Engineering career fair",
      "Over 40 companies will attend on 9 December in the main hall. Bring a printed copy of your CV." },
};

const struct Announcement *portal_announcements(int *count)
{
    if (count) *count = COUNT(ANNOUNCEMENTS);
    return ANNOUNCEMENTS;
}

double grade_points(const char *g)
{
    static const struct { const char *letter; double points; } table[] = {
        { "A+", 4.0 }, { "A", 4.0 }, { "A-", 3.7 }, { "B+", 3.3 }, { "B", 3.0 }, { "B-", 2.7 },
        { "C+", 2.3 }, { "C", 2.0 }, { "C-", 1.7 }, { "D+", 1.3 }, { "D", 1.0 }, { "F", 0.0 },
    };
    if (!g) return 0.0;
    for (int i = 0; i < COUNT(table); i++)
        if (strcmp(g, table[i].letter) == 0) return table[i].points;
    return 0.0;
}

/* ---- Authentication ----------------------------------------------------------------------- */

static int names_equal(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

/* Compares in time that depends only on the lengths, not on where the first difference is. */
static int secure_equal(const char *a, const char *b)
{
    size_t la = strlen(a), lb = strlen(b), n = la > lb ? la : lb;
    unsigned char diff = (unsigned char)(la != lb);
    for (size_t i = 0; i < n; i++) {
        unsigned char ca = i < la ? (unsigned char)a[i] : 0;
        unsigned char cb = i < lb ? (unsigned char)b[i] : 0;
        diff |= (unsigned char)(ca ^ cb);
    }
    return diff == 0;
}

const struct Student *student_authenticate(const char *name, const char *password)
{
    char clean[64];
    size_t len;
    const struct Student *match = NULL;
    if (!name || !password) return NULL;

    while (isspace((unsigned char)*name)) name++;
    len = strlen(name);
    while (len && isspace((unsigned char)name[len - 1])) len--;
    if (len == 0 || len >= sizeof(clean)) return NULL;
    memcpy(clean, name, len);
    clean[len] = '\0';

    for (int i = 0; i < COUNT(STUDENTS); i++) {
        int name_ok = names_equal(clean, STUDENTS[i]->name);
        int pass_ok = secure_equal(password, STUDENTS[i]->id);
        if (name_ok && pass_ok) match = STUDENTS[i];
    }
    return match;
}
