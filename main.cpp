#include <iostream>
#include <vector>
#include <sstream>
#include <map>
#include <regex>
#include <set>
#include <cmath>
using namespace std;

string toLower(const string &s)
{
    string result = s;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

string trim(const string &s)
{
    int start = s.find_first_not_of(" \t\r\n");
    int end = s.find_last_not_of(" \t\r\n");
    if (start == string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

vector<string> split(const string &s, char separator)
{
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, separator))
        tokens.push_back(token);
    return tokens;
}

// Structure to hold keyword matching results
struct KeywordMatchResult
{
    double score;
    vector<string> found_skills;
    vector<string> missing_skills;
};

// Structure to hold personal information extracted from a resume
struct PersonalInfo
{
    string name;
    string email;
    string phone;
    string linkedin;
    string github;
    // string portfolio;
    string codeforces;
};

// Structure to hold the complete resume analysis result
struct ResumeAnalysisResult
{
    PersonalInfo personal_info;
    int ats_score;
    // string document_type;
    KeywordMatchResult keyword_match;
    int section_score;
    int format_score;
    vector<string> education;
    vector<string> experience;
    vector<string> projects;
    vector<string> skills;
    // string summary;
    vector<string> suggestions;
    map<string, int> section_scores; // Keys: contact, summary, skills, experience, education, format
};

class ResumeAnalyzer
{
private:
    vector<string> file;

public:
    ResumeAnalyzer()
    {
        file = {"experience", "education", "skills", "work", "project", "objective",
                "summary", "employment", "qualification", "achievements"};
    }

    // Calculate how many required skills are matched in the resume text
    KeywordMatchResult calculateKeywordMatch(string resume_text, const vector<string> &required_skills)
    {
        string lower_text = toLower(resume_text);
        KeywordMatchResult result;
        for (const auto &skill : required_skills)
        {
            string skill_lower = toLower(skill);
            if (lower_text.find(skill_lower) != string::npos)
            {
                result.found_skills.push_back(skill);
            }
            else
            {
                // Check for partial match by splitting the text into sentences (using period as delimiter)
                vector<string> sentences = split(lower_text, '.');
                bool found = false;
                for (auto &sentence : sentences)
                {
                    if (sentence.find(skill_lower) != string::npos)
                    {
                        found = true;
                        break;
                    }
                }
                if (found)
                    result.found_skills.push_back(skill);
                else
                    result.missing_skills.push_back(skill);
            }
        }
        if (!required_skills.empty())
            result.score = (static_cast<double>(result.found_skills.size()) / required_skills.size()) * 100;
        else
            result.score = 0;
        return result;
    }

    // Check for essential resume sections and compute a total section score (max 100)
    int checkResumeSections(const string &text)
    {
        string lower_text = toLower(text);
        map<string, vector<string>> essential_sections = {
            {"contact", {"email", "phone", "address", "linkedin"}},
            {"education", {"education", "university", "college", "degree", "academic"}},
            {"experience", {"experience", "internship", "work", "position of responsibility"}},
            {"skills", {"skills", "technologies", "tools", "expertise"}}};
        int total_score = 0;
        for (auto &kv : essential_sections)
        {
            const vector<string> &keywords = kv.second;
            int found = 0;
            for (const auto &keyword : keywords)
            {
                if (lower_text.find(keyword) != string::npos)
                    found++;
            }
            int section_score = min(25, static_cast<int>((static_cast<double>(found) / keywords.size()) * 25));
            total_score += section_score;
        }
        return total_score;
    }

    // Check formatting and return a score (out of 100) and a list of deduction messages
    pair<int, vector<string>> checkFormatting(const string &text)
    {
        vector<string> deductions;
        int score = 100;
        // Check minimum content length
        if (text.size() < 300)
        {
            score -= 30;
            deductions.push_back("Resume is too short");
        }
        // Split text into lines
        vector<string> lines;
        istringstream iss(text);
        string line;
        while (getline(iss, line))
            lines.push_back(line);
        // Check for section headers (look for lines that are all uppercase)
        bool hasSectionHeader = false;
        for (const auto &ln : lines)
        {
            string trimmed = trim(ln);
            if (!trimmed.empty() &&
                all_of(trimmed.begin(), trimmed.end(), [](char c)
                       { return !isalpha(c) || isupper(c); }))
            {
                hasSectionHeader = true;
                break;
            }
        }
        if (!hasSectionHeader)
        {
            score -= 20;
            deductions.push_back("No clear section headers found");
        }
        // Check for bullet points in lines
        bool hasBullets = false;
        for (const auto &ln : lines)
        {
            string trimmed = trim(ln);
            if (!trimmed.empty())
            {
                if (trimmed[0] == '-' || trimmed[0] == '*') // Handle single-byte characters
                {
                    hasBullets = true;
                    break;
                }
                else if (trimmed.substr(0, 3) == "•" || trimmed.substr(0, 3) == "→") // Handle multi-byte characters
                {
                    hasBullets = true;
                    break;
                }
            }
        }
        if (!hasBullets)
        {
            score -= 20;
            deductions.push_back("No bullet points found for listing details");
        }
        // Check for inconsistent spacing: consecutive empty lines
        for (int i = 0; i < lines.size() - 1; i++)
        {
            if (trim(lines[i]).empty() && trim(lines[i + 1]).empty())
            {
                score -= 15;
                deductions.push_back("Inconsistent spacing between sections");
                break;
            }
        }
        // Check for proper contact information formatting using regex
        regex email_pattern(R"(\b[\w\.-]+@[\w\.-]+\.\w+\b)");
        regex phone_pattern(R"(\b\d{3}[-.]?\d{3}[-.]?\d{4}\b)");
        regex linkedin_pattern(R"(linkedin\.com/\w+)");
        if (!regex_search(text, email_pattern) &&
            !regex_search(text, phone_pattern) &&
            !regex_search(text, linkedin_pattern))
        {
            score -= 15;
            deductions.push_back("Missing or improperly formatted contact information");
        }
        return make_pair(max(0, score), deductions);
    }

    PersonalInfo extractPersonalInfo(const string &text)
    {
        PersonalInfo info;
        regex email_pattern(R"([\w\.-]+@[\w\.-]+\.\w+)");
        regex phone_pattern(R"((\+\d{1,3}[-.]?)?\s*\(?\d{3}\)?[-.]?\s*\d{3}[-.]?\s*\d{4})");
        regex linkedin_pattern(R"(linkedin\.com/in/[\w-]+)");
        regex github_pattern(R"(github\.com/[\w-]+)");
        regex codeforces_pattern(R"(codeforces\.com/profile/[\w-]+)");
        smatch match;
        if (regex_search(text, match, email_pattern))
            info.email = match.str(0);
        if (regex_search(text, match, phone_pattern))
            info.phone = match.str(0);
        if (regex_search(text, match, linkedin_pattern))
            info.linkedin = match.str(0);
        if (regex_search(text, match, github_pattern))
            info.github = match.str(0);
        if (regex_search(text, match, codeforces_pattern))
            info.codeforces = match.str(0);
        // Assume the first non-empty line is the candidate's name.
        istringstream iss(text);
        getline(iss, info.name);
        info.name = trim(info.name);
        if (info.name.empty())
            info.name = "Unknown";
        // info.portfolio = "";
        return info;
    }

    // Extract education section(s) from the resume text
    vector<string> extractEducation(const string &text)
    {
        vector<string> education;
        vector<string> lines;
        istringstream iss(text);
        string line;
        while (getline(iss, line))
            lines.push_back(line);
        vector<string> education_keywords = {"education", "academic", "qualification", "degree", "university", "college",
                                             "school", "institute", "certification", "diploma", "bachelor", "master",
                                             "phd", "b.tech", "m.tech", "b.e", "m.e", "b.sc", "m.sc", "bca", "mca",
                                             "b.com", "m.com", "b.cs-it", "imca", "bba", "mba", "honors", "scholarship"};
        bool inEducationSection = false;
        vector<string> current_entry;
        for (auto &ln : lines)
        {
            string trimmed = trim(ln);
            string lower_ln = toLower(trimmed);
            bool isHeader = false;
            for (auto &keyword : education_keywords)
            {
                if (lower_ln.find(keyword) != string::npos)
                {
                    isHeader = true;
                    break;
                }
            }
            if (isHeader)
            {
                // If the line is not exactly just a header, record it
                if (!trimmed.empty())
                    current_entry.push_back(trimmed);
                inEducationSection = true;
                continue;
            }
            if (inEducationSection)
            {
                bool hitOtherSection = false;
                // Check if line might belong to another section using resume keywords
                for (auto &kw : file)
                {
                    if (lower_ln.find(kw) != string::npos)
                    {
                        hitOtherSection = true;
                        break;
                    }
                }
                if (hitOtherSection)
                {
                    inEducationSection = false;
                    if (!current_entry.empty())
                    {
                        ostringstream oss;
                        for (auto &part : current_entry)
                            oss << part << " ";
                        education.push_back(oss.str());
                        current_entry.clear();
                    }
                    continue;
                }
                if (!trimmed.empty())
                    current_entry.push_back(trimmed);
                else if (!current_entry.empty())
                {
                    ostringstream oss;
                    for (auto &part : current_entry)
                        oss << part << " ";
                    education.push_back(oss.str());
                    current_entry.clear();
                }
            }
        }
        if (!current_entry.empty())
        {
            ostringstream oss;
            for (auto &part : current_entry)
                oss << part << " ";
            education.push_back(oss.str());
        }
        return education;
    }

    // Extract work experience section(s) from the resume text
    vector<string> extractExperience(const string &text)
    {
        vector<string> experience;
        vector<string> lines;
        istringstream iss(text);
        string line;
        while (getline(iss, line))
            lines.push_back(line);
        vector<string> experience_keywords = {"experience", "employment", "work history", "professional experience",
                                              "work experience", "career history", "professional background",
                                              "employment history", "job history", "positions held", "job title",
                                              "job responsibilities", "job description", "job summary"};
        bool inExperienceSection = false;
        vector<string> current_entry;
        for (auto &ln : lines)
        {
            string trimmed = trim(ln);
            string lower_ln = toLower(trimmed);
            bool isHeader = false;
            for (auto &keyword : experience_keywords)
            {
                if (lower_ln.find(keyword) != string::npos)
                {
                    isHeader = true;
                    break;
                }
            }
            if (isHeader)
            {
                if (!trimmed.empty())
                    current_entry.push_back(trimmed);
                inExperienceSection = true;
                continue;
            }
            if (inExperienceSection)
            {
                bool hitOtherSection = false;
                for (auto &kw : file)
                {
                    if (lower_ln.find(kw) != string::npos)
                    {
                        hitOtherSection = true;
                        break;
                    }
                }
                if (hitOtherSection)
                {
                    inExperienceSection = false;
                    if (!current_entry.empty())
                    {
                        ostringstream oss;
                        for (auto &part : current_entry)
                            oss << part << " ";
                        experience.push_back(oss.str());
                        current_entry.clear();
                    }
                    continue;
                }
                if (!trimmed.empty())
                    current_entry.push_back(trimmed);
                else if (!current_entry.empty())
                {
                    ostringstream oss;
                    for (auto &part : current_entry)
                        oss << part << " ";
                    experience.push_back(oss.str());
                    current_entry.clear();
                }
            }
        }
        if (!current_entry.empty())
        {
            ostringstream oss;
            for (auto &part : current_entry)
                oss << part << " ";
            experience.push_back(oss.str());
        }
        return experience;
    }

    // Extract projects section(s) from the resume text
    vector<string> extractProjects(const string &text)
    {
        vector<string> projects;
        vector<string> lines;
        istringstream iss(text);
        string line;
        while (getline(iss, line))
            lines.push_back(line);
        vector<string> project_keywords = {"projects", "personal projects", "academic projects", "key projects",
                                           "major projects", "professional projects", "project experience",
                                           "relevant projects", "featured projects", "latest projects", "top projects"};
        bool inProjectSection = false;
        vector<string> current_entry;
        for (auto &ln : lines)
        {
            string trimmed = trim(ln);
            string lower_ln = toLower(trimmed);
            bool isHeader = false;
            for (auto &keyword : project_keywords)
            {
                if (lower_ln.find(keyword) != string::npos)
                {
                    isHeader = true;
                    break;
                }
            }
            if (isHeader)
            {
                if (!trimmed.empty())
                    current_entry.push_back(trimmed);
                inProjectSection = true;
                continue;
            }
            if (inProjectSection)
            {
                bool hitOtherSection = false;
                for (auto &kw : file)
                {
                    if (lower_ln.find(kw) != string::npos)
                    {
                        hitOtherSection = true;
                        break;
                    }
                }
                if (hitOtherSection)
                {
                    inProjectSection = false;
                    if (!current_entry.empty())
                    {
                        ostringstream oss;
                        for (auto &part : current_entry)
                            oss << part << " ";
                        projects.push_back(oss.str());
                        current_entry.clear();
                    }
                    continue;
                }
                if (!trimmed.empty())
                    current_entry.push_back(trimmed);
                else if (!current_entry.empty())
                {
                    ostringstream oss;
                    for (auto &part : current_entry)
                        oss << part << " ";
                    projects.push_back(oss.str());
                    current_entry.clear();
                }
            }
        }
        if (!current_entry.empty())
        {
            ostringstream oss;
            for (auto &part : current_entry)
                oss << part << " ";
            projects.push_back(oss.str());
        }
        return projects;
    }

    // Perform the overall resume analysis given the raw text and job requirements (required skills, GPA requirement, etc.)
    ResumeAnalysisResult analyzeResume(const string &raw_text, const vector<string> &required_skills, bool require_gpa = false)
    {
        ResumeAnalysisResult result;
        result.personal_info = extractPersonalInfo(raw_text);

        result.keyword_match = calculateKeywordMatch(raw_text, required_skills);
        result.education = extractEducation(raw_text);
        result.experience = extractExperience(raw_text);
        result.projects = extractProjects(raw_text);

        result.section_score = checkResumeSections(raw_text);
        auto formatResult = checkFormatting(raw_text);
        result.format_score = formatResult.first;
        vector<string> format_deductions = formatResult.second;

        // Generate suggestions for contact information
        vector<string> contact_suggestions;
        if (result.personal_info.email.empty())
            contact_suggestions.push_back("Add your email address");
        if (result.personal_info.phone.empty())
            contact_suggestions.push_back("Add your phone number");
        if (result.personal_info.linkedin.empty())
            contact_suggestions.push_back("Add your LinkedIn profile URL");

        // Generate suggestions for skills section
        vector<string> skills_suggestions;
        if (!result.keyword_match.missing_skills.empty())
        {
            skills_suggestions.push_back("Mising skills are: ");
            for (auto &it : result.keyword_match.missing_skills)
            {
                skills_suggestions.push_back(it);
            }
        }

        // Generate suggestions for experience section
        vector<string> experience_suggestions;
        if (result.experience.empty())
        {
            experience_suggestions.push_back("Add your work experience section");
        }
        else
        {
            bool has_dates = false, has_bullets = false, has_action_verbs = false;
            regex date_pattern(R"(\b(19|20)\d{2}\b)");
            regex bullet_pattern(R"([•\-\*])");
            regex action_verbs(R"(\b(developed|managed|created|implemented|designed|led|improved)\b)");
            for (auto &exp : result.experience)
            {
                if (regex_search(exp, date_pattern))
                    has_dates = true;
                if (regex_search(exp, bullet_pattern))
                    has_bullets = true;
                if (regex_search(toLower(exp), action_verbs))
                    has_action_verbs = true;
            }
            if (!has_dates)
                experience_suggestions.push_back("Include dates for each work experience");
            if (!has_bullets)
                experience_suggestions.push_back("Use bullet points to list your achievements and responsibilities");
            if (!has_action_verbs)
                experience_suggestions.push_back("Start bullet points with strong action verbs");
        }

        // Generate suggestions for education section
        vector<string> education_suggestions;
        if (result.education.empty())
        {
            education_suggestions.push_back("Add your educational background");
        }
        else
        {
            bool has_dates = false, has_degree = false, has_gpa = false;
            regex date_pattern(R"(\b(19|20)\d{2}\b)");
            regex degree_pattern(R"(\b(bachelor|master|phd|b\.|m\.|diploma)\b)");
            regex gpa_pattern(R"(\b(gpa|cgpa|grade|percentage)\b)");
            for (auto &edu : result.education)
            {
                if (regex_search(edu, date_pattern))
                    has_dates = true;
                if (regex_search(toLower(edu), degree_pattern))
                    has_degree = true;
                if (regex_search(toLower(edu), gpa_pattern))
                    has_gpa = true;
            }
            if (!has_dates)
                education_suggestions.push_back("Include graduation dates");
            if (!has_degree)
                education_suggestions.push_back("Specify your degree type");
            if (!has_gpa && require_gpa)
                education_suggestions.push_back("Include your CGPA if it's above 7.0");
        }

        // Formatting suggestions
        vector<string> format_suggestions;
        if (result.format_score < 100)
            format_suggestions = format_deductions;

        int contact_score = 100 - (contact_suggestions.size() * 25);

        int skills_score = static_cast<int>(result.keyword_match.score);
        int experience_score = 100 - (experience_suggestions.size() * 25);
        int education_score = 100 - (education_suggestions.size() * 25);

        int ats_score = static_cast<int>(round(contact_score * 0.1)) +
                        static_cast<int>(round(skills_score * 0.35)) +
                        static_cast<int>(round(experience_score * 0.25)) +
                        static_cast<int>(round(education_score * 0.1)) +
                        static_cast<int>(round(result.format_score * 0.2));
        result.ats_score = ats_score;

        // Combine all suggestions
        result.suggestions.insert(result.suggestions.end(), contact_suggestions.begin(), contact_suggestions.end());
        result.suggestions.insert(result.suggestions.end(), skills_suggestions.begin(), skills_suggestions.end());
        result.suggestions.insert(result.suggestions.end(), experience_suggestions.begin(), experience_suggestions.end());
        result.suggestions.insert(result.suggestions.end(), education_suggestions.begin(), education_suggestions.end());
        result.suggestions.insert(result.suggestions.end(), format_suggestions.begin(), format_suggestions.end());
        if (result.suggestions.empty())
            result.suggestions.push_back("Your resume is well-optimized for ATS systems");

        result.section_scores["contact"] = contact_score;
        result.section_scores["skills"] = skills_score;
        result.section_scores["experience"] = experience_score;
        result.section_scores["education"] = education_score;
        result.section_scores["format"] = result.format_score;

        return result;
    }
};

int main()
{
    // Demonstration: a sample resume text and required skills
    string resumeText = "Tanmya Potdar\n"
                        "Email: 2021mcb1252@iitrpr.ac.in\n"
                        "Phone: 6362127519\n"
                        "LinkedIn: linkedin.com/in/johndoe\n"
                        "\n"
                        "PROFESSIONAL SUMMARY\n"
                        "Experienced software developer with expertise in C++ and Python.\n"
                        "\n"
                        // "EXPERIENCE\n"
                        // "Software Developer at XYZ Corp (2018 - 2021)\n"
                        // "Developed applications using C++ and Python.\n"

                        "EDUCATION\n"
                        "•Bachelor of Science in Computer Science from college IIT Ropar (2014 - 2018)\n"
                        "\n"
                        "SKILLS\n"
                        "C++, PythonL\n"
                        "\n"
                        "PROJECTS\n"
                        "• Weather Application Apr. 2023"
                        "Vue|Tailwind CSS Github"
                        "– Created a weather application which tells about the weather and all the related details for any city using Vue and Tailwind"
                        "CSS. Used Mapbox API for weather information retrieval and location tracking"
                        "– Functionalities include tracking a city, accessing weather data for the next 10 days, add/delete city, etc.";

    vector<string> required_skills = {"C++", "Python", "SQL", "Java"};

    ResumeAnalyzer analyzer;
    ResumeAnalysisResult result = analyzer.analyzeResume(resumeText, required_skills, true);

    cout << "ATS Score: " << result.ats_score << "\n";

    cout << "Name: " << result.personal_info.name << "\n";
    cout << "Email: " << result.personal_info.email << "\n";
    cout << "Phone: " << result.personal_info.phone << "\n";
    cout << "LinkedIn: " << result.personal_info.linkedin << "\n\n";
    cout << "Suggestions:\n";
    for (auto &suggestion : result.suggestions)
    {
        cout << "- " << suggestion << "\n";
    }
    return 0;
}