REVOKE CREATE ON SCHEMA public FROM PUBLIC;
CREATE ROLE backend LOGIN;
GRANT SELECT, INSERT ON ALL TABLES IN SCHEMA public TO backend;
GRANT USAGE ON ALL SEQUENCES IN SCHEMA public TO backend;
-- remember to set password with \password backend
