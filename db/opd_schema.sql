--
-- PostgreSQL database dump
--

SET client_encoding = 'SQL_ASCII';
SET standard_conforming_strings = off;
SET check_function_bodies = false;
SET client_min_messages = warning;
SET escape_string_warning = off;

SET search_path = public, pg_catalog;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: jobs; Type: TABLE; Schema: public; Owner: opd; Tablespace:
--

CREATE TABLE jobs (
    id integer NOT NULL,
    job_name text,
    queueid text,
    start_time timestamp without time zone,
    end_time timestamp without time zone,
    exec_name text
);


ALTER TABLE public.jobs OWNER TO opd;

--
-- Name: jobs_id_seq; Type: SEQUENCE; Schema: public; Owner: opd
--

CREATE SEQUENCE jobs_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.jobs_id_seq OWNER TO opd;

--
-- Name: jobs_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: opd
--

ALTER SEQUENCE jobs_id_seq OWNED BY jobs.id;


--
-- Name: jobs_id_seq; Type: SEQUENCE SET; Schema: public; Owner: opd
--

SELECT pg_catalog.setval('jobs_id_seq', 265, true);


--
-- Name: messages; Type: TABLE; Schema: public; Owner: opd; Tablespace:
--

CREATE TABLE messages (
    id integer NOT NULL,
    job_id integer,
    rank integer,
    tstamp timestamp without time zone,
    key text,
    value text
);


ALTER TABLE public.messages OWNER TO opd;

--
-- Name: messages_id_seq; Type: SEQUENCE; Schema: public; Owner: opd
--

CREATE SEQUENCE messages_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.messages_id_seq OWNER TO opd;

--
-- Name: messages_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: opd
--

ALTER SEQUENCE messages_id_seq OWNED BY messages.id;


--
-- Name: messages_id_seq; Type: SEQUENCE SET; Schema: public; Owner: opd
--

SELECT pg_catalog.setval('messages_id_seq', 4045841, true);


--
-- Name: schema_info; Type: TABLE; Schema: public; Owner: opd; Tablespace:
--

CREATE TABLE schema_info (
    version integer
);


ALTER TABLE public.schema_info OWNER TO opd;

--
-- Name: id; Type: DEFAULT; Schema: public; Owner: opd
--

ALTER TABLE jobs ALTER COLUMN id SET DEFAULT nextval('jobs_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: opd
--

ALTER TABLE messages ALTER COLUMN id SET DEFAULT nextval('messages_id_seq'::regclass);


--
-- Name: jobs_pkey; Type: CONSTRAINT; Schema: public; Owner: opd; Tablespace:
--

ALTER TABLE ONLY jobs
    ADD CONSTRAINT jobs_pkey PRIMARY KEY (id);


--
-- Name: messages_pkey; Type: CONSTRAINT; Schema: public; Owner: opd; Tablespace:
--

ALTER TABLE ONLY messages
    ADD CONSTRAINT messages_pkey PRIMARY KEY (id);


--
-- Name: messages_jobid_idx; Type: INDEX; Schema: public; Owner: opd; Tablespace:
--

CREATE INDEX messages_jobid_idx ON messages USING btree (job_id);


--
-- Name: messages_key_idx; Type: INDEX; Schema: public; Owner: opd; Tablespace:
--

CREATE INDEX messages_key_idx ON messages USING btree ("key");


--
-- Name: messages_job_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: opd
--

ALTER TABLE ONLY messages
    ADD CONSTRAINT messages_job_id_fkey FOREIGN KEY (job_id) REFERENCES jobs(id) ON DELETE CASCADE;


--
-- Name: public; Type: ACL; Schema: -; Owner: postgres
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM postgres;
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- PostgreSQL database dump complete
--

