#include <QTextStream>
#include <QDebug>
#include "paramfile.h"
#include "pixytypes.h"

ParamFile::ParamFile()
{
    m_doc = NULL;
    m_file = NULL;
}

ParamFile::~ParamFile()
{
    close();
}


int ParamFile::open(const QString &filename, bool read)
{
    m_read = read;
    m_file = new QFile(filename);
    m_doc = new QDomDocument;
    if (m_read)
    {
        QString error;
        int line, col;
        if (!m_file->open(QIODevice::ReadOnly | QIODevice::Text))
            return -1;
        if (!m_doc->setContent(m_file, &error, &line, &col))
            return -2;
    }
    else // write
    {
        if (!m_file->open(QIODevice::WriteOnly | QIODevice::Text))
            return -1;
    }

    return 0;
}

int ParamFile::write(const QString &tag, ParameterDB *data)
{
    int i;

    QDomElement element = m_doc->createElement(tag);

    if (data)
    {
        Parameters &parameters = data->parameters();
        for (i=0; i<parameters.size(); i++)
        {
            QDomElement item = m_doc->createElement("data");
            PType type = parameters[i].type();
            uint flags;
            if (parameters[i].property(PP_FLAGS).isNull())
                flags = 0;
            else
                flags = parameters[i].property(PP_FLAGS).toUInt(); // embedded flags

            item.setAttribute("key", parameters[i].id());
            item.setAttribute("type", parameters[i].typeName());
            if (type==PT_RADIO)
                item.setAttribute("value", *parameters[i].description());
            if (type==PT_INTS8)
            {
                QByteArray a = parameters[i].value().toByteArray();
                a = a.toBase64();
                item.setAttribute("value", QString(a));
            }
            else if (type==PT_INT32 || type==PT_INT16 || type==PT_INT8)
            {
                if (flags&PRM_FLAG_SIGNED)
                {
                    int val = parameters[i].valueInt();
                    item.setAttribute("value", QString::number(val));
                }
                else
                {
                    uint val = parameters[i].value().toUInt();
                    item.setAttribute("value", QString::number(val));
                }
            }
            else // handle string and float
                item.setAttribute("value", parameters[i].value().toString());
            element.appendChild(item);
        }
    }

    m_doc->appendChild(element);
    return 0;
}

int ParamFile::read(const QString &tag, ParameterDB *data)
{
    QDomElement element, nextElement;
    QDomNode node;

    element = m_doc->firstChildElement(tag);
    node = element.firstChild();
    nextElement = node.toElement();

    while(!nextElement.isNull())
    {
        QString key;
        QString type;
        QString value;

        key = nextElement.attribute("key");
        type = nextElement.attribute("type");
        value = nextElement.attribute("value");

        Parameter parameter(key, Parameter::typeLookup(type));

        // always set dirty state
        parameter.setDirty(true);

        PType ptype = Parameter::typeLookup(type);

        if (ptype==PT_RADIO)
            parameter.setRadio(value);
        else
        {
            if (ptype==PT_FLT32)
            {
                float val = value.toFloat();
                parameter.set(val);
            }
            else if (ptype==PT_INTS8)
            {
                QByteArray a = value.toUtf8();
                a = QByteArray::fromBase64(a);
                parameter.set(QVariant(a));
            }
            else if (ptype==PT_INT8 || ptype==PT_INT16 || ptype==PT_INT32)
            {
                int val = value.toInt();
                parameter.set(val);
            }
            else // all other cases (STRING)
                parameter.set(value);
        }
        data->add(parameter);

        node = nextElement.nextSibling();
        nextElement = node.toElement();
    }

    return 0;
}


void ParamFile::close()
{
    if (m_doc && m_file)
    {
        if (!m_read)
        {
            QTextStream out(m_file);
            out << m_doc->toString();
        }

        m_file->close();

        delete m_file;
        delete m_doc;
        m_file = NULL;
        m_doc = NULL;
    }
}


