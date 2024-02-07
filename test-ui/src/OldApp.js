import React, { useEffect, useState, useRef } from 'react';
import axios from 'axios';
import Ticker from 'react-ticker'

function Test() {
    const [sections, setSections] = useState([]);

    useEffect(() => {
        const fetchText = () => {
            axios.get('http://127.0.0.1:5000/api/text')
                .then(response => {
                    setSections(response.data);
                })
                .catch(error => console.error('There was an error!', error));
        };

        const intervalId = setInterval(fetchText, 200); // Update every 200ms

        return () => clearInterval(intervalId);
    }, []);

    const renderSection = (section) => {
        const modifiedText = section.text
            .replace("[URL1]", section.url1 ? `<img src="${section.url1}" alt="Image 1" style="height: 1em; display: inline; vertical-align: middle;"/>` : "")
            .replace("[URL2]", section.url2 ? `<img src="${section.url2}" alt="Image 2" style="height: 1em; display: inline; vertical-align: middle;"/>` : "");

        return <span key={section.id} dangerouslySetInnerHTML={{ __html: modifiedText }} />;
    };

    return (
        <Ticker offset="run-in" speed={10}>
            {sections.map(renderSection)}
        </Ticker>
    );
}

export default Test;
